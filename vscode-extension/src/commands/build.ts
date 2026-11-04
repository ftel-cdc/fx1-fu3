// build.ts - build target dang chon (khong flash). Tu dong configure neu
// build dir chua ton tai (thay the task "CMake: configure (...)" da xoa).
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { runCMakeTarget } from "../cmakeRunner";
import { ensureConfigured } from "../configure";

export async function buildCommand(state: NeonMdfState): Promise<void> {
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
    taskName: `NEON-MDF: build (${state.target})`,
    preset: state.target,
    cwd: folder.uri,
  });

  if (exitCode !== 0) {
    vscode.window.showErrorMessage(`NEON-MDF: build that bai (target ${state.target}).`);
  }
}
