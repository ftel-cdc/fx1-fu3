// cmakeRunner.ts - chay `cmake --build --preset <preset> [--target <t>]`
// qua VS Code Task API (khong phai child_process truc tiep) de tai dung
// terminal panel/problem-matcher/huy-task co san, giong cach .vscode/tasks.json
// cua cac vi du da lam thu cong - day la diem DUY NHAT trong extension goi
// CMake, moi command (build/flash/buildFlashMonitor) deu di qua day.
import * as vscode from "vscode";
import { sdkPathEnv } from "./sdkPath";

const TASK_SOURCE = "NEON-MDF";

export interface RunCMakeTargetOptions {
  taskName: string;
  preset: string;
  /** Bo trong neu muon build target mac dinh (`all`). */
  cmakeTarget?: string;
  cwd: vscode.Uri;
  /** Bien moi truong bo sung cho shell chay lenh - vd NEON_MDF_SERIAL_PORT. */
  env?: Record<string, string>;
  background?: boolean;
}

/**
 * Tra ve exit code THAT cua tien trinh (khong phai khi task chi "bat dau") -
 * cac command phia tren dua vao gia tri nay de biet build/flash thanh cong
 * hay khong va hien thong bao loi tuong ung.
 */
export function runCMakeTarget(options: RunCMakeTargetOptions): Promise<number | undefined> {
  const args = ["--build", "--preset", options.preset];
  if (options.cmakeTarget) {
    args.push("--target", options.cmakeTarget);
  }

  const shellExec = new vscode.ShellExecution("cmake", args, {
    cwd: options.cwd.fsPath,
    // sdkPathEnv() chi tra ve gia tri neu nguoi dung da chon SDK path rieng
    // (neonMdf.sdkPath) - de trong thi khong can, de CMakeLists.txt/
    // settings.json cua tung app tu quyet dinh nhu truoc gio.
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

/** `cmake --preset <preset>` (chi configure, khong build) - xem configure.ts cho logic "khi nao can goi ham nay". */
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
