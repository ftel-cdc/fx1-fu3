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

export async function configure(app: Application, force = false): Promise<void> {
  if (!force) {
    try {
      await vscode.workspace.fs.stat(vscode.Uri.joinPath(app.uri, "build", PRESET, "CMakeCache.txt"));
      return;
    } catch (error) {
      if (!(error instanceof vscode.FileSystemError) || error.code !== "FileNotFound") {
        throw error;
      }
    }
  }
  await requireSuccess(runTask(app, "Configure", ["--preset", PRESET]), "Configuration");
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
