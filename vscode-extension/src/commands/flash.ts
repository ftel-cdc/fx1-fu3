// Program GT32U2086 through the ROMCODE UART service.
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { runCMakeTarget } from "../cmakeRunner";
import { ensureConfigured } from "../configure";
import { NeonMdfSerialMonitor } from "../serialMonitor/monitor";

export async function flashCommand(state: NeonMdfState, monitor: NeonMdfSerialMonitor): Promise<void> {
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
  await runFlashUart(state, folder.uri);
}

export async function runFlashUart(state: NeonMdfState, folder: vscode.Uri): Promise<number | undefined> {
  if (state.target !== "GT32U2086") {
    vscode.window.showErrorMessage("NEON-MDF: Select target GT32U2086.");
    return undefined;
  }
  if (!state.port) {
    vscode.window.showWarningMessage("NEON-MDF: Select the board serial port before flashing.");
    return undefined;
  }
  if (!Number.isInteger(state.flashBaudRate) || state.flashBaudRate <= 0) {
    vscode.window.showErrorMessage("NEON-MDF: neonMdf.gt32u2086FlashBaudRate must be a positive integer matching ROMCODE.");
    return undefined;
  }
  const exitCode = await runCMakeTarget({
    taskName: "NEON-MDF: GT32U2086 ROMCODE UART update",
    preset: state.target,
    cmakeTarget: "flash_uart_gt32u2086",
    cwd: folder,
    env: {
      NEON_MDF_SERIAL_PORT: state.port,
      NEON_MDF_SERIAL_BAUD: String(state.flashBaudRate),
    },
  });
  if (exitCode !== 0 && exitCode !== undefined) {
    vscode.window.showErrorMessage("NEON-MDF: UART flash failed. Check boot mode 100, RESET and ROMCODE baud; see terminal.");
  }
  return exitCode;
}
