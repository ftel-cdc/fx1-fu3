// menuconfig.ts - wrapper mong goi target CMake dung chung `menuconfig`
// (xem neon-mdf/cmake/kconfig.cmake) - logic that nam ben do, khong lap o
// day. Thay the task "menuconfig (bat/tat tinh nang...)" da co trong
// tasks.json cu.
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { ensureConfigured } from "../configure";
import { runCMakeTarget } from "../cmakeRunner";

export async function menuconfigCommand(state: NeonMdfState): Promise<void> {
  const folder = vscode.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode.window.showWarningMessage("NEON-MDF: chua mo workspace folder nao.");
    return;
  }
  if (!state.target) {
    vscode.window.showWarningMessage(
      'NEON-MDF: chua chon target - bam vao "chon target" tren status bar truoc.'
    );
    return;
  }

  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode.window.showErrorMessage(`NEON-MDF: configure that bai (target ${state.target}) - xem terminal.`);
    return;
  }

  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: menuconfig (${state.target})`,
    preset: state.target,
    cmakeTarget: "menuconfig",
    cwd: folder.uri,
  });

  if (exitCode !== 0) {
    vscode.window.showErrorMessage(`NEON-MDF: menuconfig that bai (target ${state.target}) - xem terminal.`);
  }
}
