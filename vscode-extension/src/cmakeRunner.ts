import { randomUUID } from "node:crypto";
import * as vscode from "vscode";
import { Application } from "./application";
import { sdkPathEnv } from "./sdkPath";

const PRESET = "GT32U2086";

function runTask(
  app: Application,
  name: string,
  args: string[],
  env: Record<string, string> = {},
): Promise<number | undefined> {
  const id = randomUUID();
  const task = new vscode.Task(
    { type: "neon-mdf", id },
    app.folder,
    name,
    "NEON-MDF",
    new vscode.ProcessExecution("cmake", args, {
      cwd: app.uri.fsPath,
      env: { ...sdkPathEnv(app.uri), ...env },
    }),
    [],
  );
  task.presentationOptions = {
    reveal: vscode.TaskRevealKind.Always,
    panel: vscode.TaskPanelKind.Dedicated,
  };

  return new Promise((resolve, reject) => {
    const processEnd = vscode.tasks.onDidEndTaskProcess((event) => {
      if (event.execution.task.definition.id === id) {
        cleanup();
        resolve(event.exitCode);
      }
    });
    const taskEnd = vscode.tasks.onDidEndTask((event) => {
      if (event.execution.task.definition.id === id) {
        cleanup();
        resolve(undefined);
      }
    });

    function cleanup() {
      processEnd.dispose();
      taskEnd.dispose();
    }

    vscode.tasks.executeTask(task).then(undefined, (error) => {
      cleanup();
      reject(error);
    });
  });
}

// Restore user options that must survive a fresh configure. CMake regenerates
// compiler detection, paths derived from the preset and other internal entries.
const preservedOptions = new Set([
  "Python3_EXECUTABLE", "NEON_MDF_PATH", "NEON_MDF_PROFILE", "NEON_MDF_SECURE_BOOT",
  "NEON_MDF_SIGNING_KEY", "NEON_MDF_GT32U2086_APP_VERSION",
]);

async function freshOptions(app: Application, cache: vscode.Uri): Promise<string[]> {
  try {
    const content = Buffer.from(await vscode.workspace.fs.readFile(cache)).toString("utf8");
    const options: string[] = [];
    for (const line of content.split(/\r?\n/)) {
      const entry = /^([A-Za-z0-9_]+):([^=]+)=(.*)$/.exec(line);
      if (entry && entry[2] !== "INTERNAL" && preservedOptions.has(entry[1]) &&
          (entry[1] !== "NEON_MDF_PATH" || !sdkPathEnv(app.uri))) {
        options.push(`-D${entry[1]}=${entry[3]}`);
      }
    }
    return options;
  } catch (error) {
    if (error instanceof vscode.FileSystemError && error.code === "FileNotFound") {
      return [];
    }
    throw error;
  }
}

export async function configure(
  app: Application,
  mode: "if-needed" | "refresh" | "fresh" = "if-needed",
): Promise<void> {
  const dir = vscode.Uri.joinPath(app.uri, "build", PRESET);
  const cache = vscode.Uri.joinPath(dir, "CMakeCache.txt");
  if (mode === "if-needed") {
    try {
      await vscode.workspace.fs.stat(cache);
      await vscode.workspace.fs.stat(vscode.Uri.joinPath(dir, "build.ninja"));
      return;
    } catch (error) {
      if (!(error instanceof vscode.FileSystemError) || error.code !== "FileNotFound") {
        throw error;
      }
    }
  }
  const args = ["--preset", PRESET];
  if (mode === "fresh") {
    args.push(...await freshOptions(app, cache), "--fresh");
  }
  await requireSuccess(runTask(app, "Configure", args), "Configuration");
}

export async function runBuild(
  app: Application,
  name: string,
  target?: string,
  env?: Record<string, string>,
): Promise<void> {
  const args = ["--build", "--preset", PRESET];
  if (target) {
    args.push("--target", target);
  }
  await requireSuccess(runTask(app, name, args, env), name);
}

async function requireSuccess(result: Promise<number | undefined>, action: string): Promise<void> {
  const exitCode = await result;
  if (exitCode !== 0) {
    throw new Error(exitCode === undefined
      ? `${action} ended without a result. Check the task terminal.`
      : `${action} failed (exit code ${exitCode}). Check the task terminal.`);
  }
}
