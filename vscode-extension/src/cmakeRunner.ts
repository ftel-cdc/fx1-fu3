// Shared CMake task runner. Commands use VS Code tasks for consistent terminal
// output, problem matching and cancellation.
import * as vscode from "vscode";
import { sdkPathEnv } from "./sdkPath";

const TASK_SOURCE = "NEON-MDF";

export interface RunCMakeTargetOptions {
  taskName: string;
  preset: string;
  /** set in if you want to build default target (`all`). */
  cmakeTarget?: string;
  cwd: vscode.Uri;
  /** variable of each plugin field for the command shell - e.g. NEON_MDF_SERIAL_PORT. */
  env?: Record<string, string>;
  background?: boolean;
}

/**
 * look for the actual exit code of the program (not when the task just "starts") - the command above
 * only has this value to know if build/flash succeeds or fails and displays a similar error
 * message.
 */
export function runCMakeTarget(options: RunCMakeTargetOptions): Promise<number | undefined> {
  const args = ["--build", "--preset", options.preset];
  if (options.cmakeTarget) {
    args.push("--target", options.cmakeTarget);
  }

  const shellExec = new vscode.ShellExecution("cmake", args, {
    cwd: options.cwd.fsPath,
    // sdkPathEnv() only checks the value if the user has selected a separate SDK path (neonMdf.sdkPath) -
    // it is not necessary to leave in, so that CMakeLists.txt/ settings.json of each app from the
    // specified as before.
    env: { ...sdkPathEnv(), ...options.env },
  });

  const task = new vscode.Task(
    { type: TASK_SOURCE, target: options.cmakeTarget ?? "all" },
    vscode.TaskScope.Workspace,
    options.taskName,
    TASK_SOURCE,
    shellExec,
    []
  );
  task.presentationOptions = {
    reveal: vscode.TaskRevealKind.Always,
    panel: vscode.TaskPanelKind.Dedicated,
  };
  if (options.background) {
    task.isBackground = true;
  }

  return new Promise((resolve) => {
    const disposable = vscode.tasks.onDidEndTaskProcess((e) => {
      if (e.execution.task === task) {
        disposable.dispose();
        resolve(e.exitCode);
      }
    });
    vscode.tasks.executeTask(task);
  });
}

/** `cmake --preset <preset>` (configure only, not build) - see configure.ts for "when to call" logic. */
export function runCMakeConfigure(preset: string, cwd: vscode.Uri): Promise<number | undefined> {
  const shellExec = new vscode.ShellExecution("cmake", ["--preset", preset], {
    cwd: cwd.fsPath,
    env: sdkPathEnv(),
  });

  const task = new vscode.Task(
    { type: TASK_SOURCE, target: "configure" },
    vscode.TaskScope.Workspace,
    `NEON-MDF: configure (${preset})`,
    TASK_SOURCE,
    shellExec,
    []
  );
  task.presentationOptions = {
    reveal: vscode.TaskRevealKind.Always,
    panel: vscode.TaskPanelKind.Dedicated,
  };

  return new Promise((resolve) => {
    const disposable = vscode.tasks.onDidEndTaskProcess((e) => {
      if (e.execution.task === task) {
        disposable.dispose();
        resolve(e.exitCode);
      }
    });
    vscode.tasks.executeTask(task);
  });
}
