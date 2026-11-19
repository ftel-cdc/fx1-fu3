import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { ensureConfigured } from "../configure";
import { runFlashUart } from "./flash";
import { NeonMdfSerialMonitor } from "../serialMonitor/monitor";

export async function buildFlashMonitorCommand(state: NeonMdfState, monitor: NeonMdfSerialMonitor): Promise<void> {
  const folder = vscode.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode.window.showWarningMessage("NEON-MDF: Open an application folder first.");
    return;
  }
  if (!(await ensureConfigured(folder.uri, state.target))) {
    vscode.window.showErrorMessage("NEON-MDF: Configuration failed; see terminal.");
    return;
  }
  await monitor.close();
  const exitCode = await runFlashUart(state, folder.uri);
  if (exitCode === 0) {
    await monitor.open(state);
  }
}
