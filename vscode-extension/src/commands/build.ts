// build.ts - selected build target (not flash). automatically configures if build dir does not already
// exist (see the task "CMake: configure (...)" erased).
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { runCMakeTarget } from "../cmakeRunner";
import { ensureConfigured } from "../configure";

export async function buildCommand(state: NeonMdfState): Promise<void> {
  const folder = vscode.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode.window.showWarningMessage("NEON-MDF: no workspace folder opened.");
    return;
  }
  if (!state.target) {
    vscode.window.showWarningMessage(
      'NEON-MDF: No target selected. Use Select Target in the status bar first.'
    );
    return;
  }

  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode.window.showErrorMessage(`NEON-MDF: configure failed (target ${state.target}) - see terminal.`);
    return;
  }

  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: build (${state.target})`,
    preset: state.target,
    cwd: folder.uri,
  });

  if (exitCode !== 0) {
    vscode.window.showErrorMessage(`NEON-MDF: build failed (target ${state.target}).`);
  }
}
