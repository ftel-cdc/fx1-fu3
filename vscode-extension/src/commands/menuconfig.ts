// menuconfig.ts - wrapper expects CMake target to be shared `menuconfig` (see
// neon-mdf/cmake/kconfig.cmake). The implementation lives there; see the "menuconfig" task
// (feature toggles, etc.)" already in
// legacy tasks.json.
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { ensureConfigured } from "../configure";
import { runCMakeTarget } from "../cmakeRunner";

export async function menuconfigCommand(state: NeonMdfState): Promise<void> {
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
    taskName: `NEON-MDF: menuconfig (${state.target})`,
    preset: state.target,
    cmakeTarget: "menuconfig",
    cwd: folder.uri,
  });

  if (exitCode !== 0) {
    vscode.window.showErrorMessage(`NEON-MDF: menuconfig failed (target ${state.target}) - see terminal.`);
  }
}
