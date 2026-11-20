import { randomUUID } from "node:crypto";
import * as vscode from "vscode";
import { Application } from "./application";
import { resolveSdkPath, validateSdkPath } from "./sdkPath";

const PRESET = "GT32U2086";

function runTask(
  app: Application,
  name: string,
  args: string[],
  sdkDir?: vscode.Uri,
  env: Record<string, string> = {},
): Promise<number | undefined> {
  const id = randomUUID();
  // When the application must resolve its own SDK, do not forward a stale
  // NEON_MDF_PATH inherited from the task environment.
  const taskArgs = sdkDir ? args : ["-E", "env", "--unset=NEON_MDF_PATH", "cmake", ...args];
  const task = new vscode.Task(
    { type: "neon-mdf", id },
    app.folder,
    name,
    "NEON-MDF",
    new vscode.ProcessExecution("cmake", taskArgs, {
      cwd: app.uri.fsPath,
      env: { ...env, ...(sdkDir ? { NEON_MDF_PATH: sdkDir.fsPath } : {}) },
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

async function freshOptions(app: Application, cache: vscode.Uri, sdkDir?: vscode.Uri): Promise<string[]> {
  try {
    const content = Buffer.from(await vscode.workspace.fs.readFile(cache)).toString("utf8");
    const options: string[] = [];
    for (const line of content.split(/\r?\n/)) {
      const entry = /^([A-Za-z0-9_]+):([^=]+)=(.*)$/.exec(line);
      if (entry && entry[2] !== "INTERNAL" && preservedOptions.has(entry[1]) &&
          (entry[1] !== "NEON_MDF_PATH" || (!sdkDir && await validateSdkPath(app.uri, entry[3])))) {
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
  mode: "refresh" | "fresh",
  sdkPathOverride?: string,
): Promise<void> {
  const dir = vscode.Uri.joinPath(app.uri, "build", PRESET);
  const cache = vscode.Uri.joinPath(dir, "CMakeCache.txt");
  const sdkDir = (await resolveSdkPath(app.uri, sdkPathOverride))?.uri;
  const args = ["--preset", PRESET];
  if (mode === "fresh") {
    const options = await freshOptions(app, cache, sdkDir);
    if (!options.some((option) => option.startsWith("-DPython3_EXECUTABLE="))) {
      const python = await findVirtualEnvPython(app, sdkDir);
      if (python) {
        options.push(`-DPython3_EXECUTABLE=${python}`);
      }
    }
    args.push(...options, "--fresh");
  }
  await requireSuccess(runTask(app, "Configure", args, sdkDir), "Configuration");
}

async function findVirtualEnvPython(app: Application, sdkDir?: vscode.Uri): Promise<string | undefined> {
  const roots = [app.uri, app.folder.uri];
  if (sdkDir) {
    roots.push(vscode.Uri.joinPath(sdkDir, ".."));
  }
  const executable = process.platform === "win32" ? ["Scripts", "python.exe"] : ["bin", "python"];
  for (const root of roots) {
    const candidate = vscode.Uri.joinPath(root, ".venv", ...executable);
    try {
      const stat = await vscode.workspace.fs.stat(candidate);
      if ((stat.type & vscode.FileType.File) !== 0) {
        return candidate.fsPath;
      }
    } catch (error) {
      if (!(error instanceof vscode.FileSystemError) || error.code !== "FileNotFound") {
        throw error;
      }
    }
  }
}

export async function runBuild(
  app: Application,
  name: string,
  sdkPathOverride?: string,
  target?: string,
  env?: Record<string, string>,
): Promise<void> {
  const args = ["--build", "--preset", PRESET];
  if (target) {
    args.push("--target", target);
  }
  const sdkDir = (await resolveSdkPath(app.uri, sdkPathOverride))?.uri;
  await requireSuccess(runTask(app, name, args, sdkDir, env), name);
}

async function requireSuccess(result: Promise<number | undefined>, action: string): Promise<void> {
  const exitCode = await result;
  if (exitCode !== 0) {
    throw new Error(exitCode === undefined
      ? `${action} ended without a result. Check the task terminal.`
      : `${action} failed (exit code ${exitCode}). Check the task terminal.`);
  }
}
