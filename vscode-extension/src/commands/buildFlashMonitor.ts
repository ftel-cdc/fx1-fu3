// Share the Flash dispatcher, release the serial port before programming,
// then monitor only after success. FU2 ROMCODE hands off to the application;
// STM32 System Bootloader retains its board-specific reset instructions.
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { runCMakeTarget } from "../cmakeRunner";
import { ensureConfigured } from "../configure";
import { runFlashCombined, runFlashUart } from "./flash";
import { NeonMdfSerialMonitor } from "../serialMonitor/monitor";

export async function buildFlashMonitorCommand(state: NeonMdfState, monitor: NeonMdfSerialMonitor): Promise<void> {
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

  if (!state.port) {
    vscode.window.showWarningMessage(
      'NEON-MDF: not selected serial port - needed for both flash (UART path) times serial monitor.'
    );
    return;
  }

  await monitor.close();
  let exitCode: number | undefined;
  if (state.transport === "jtag") {
    exitCode = await runFlashCombined(state, folder.uri);
  } else {
    exitCode = await runFlashUart(state, folder.uri);
    if (exitCode === 0 && state.target !== "FU2") {
      const proceed = await vscode.window.showInformationMessage(
        "NEON-MDF: Have you released B0, disconnected PB2 from GND and reset the board? Complete these steps to start the app.",
        "Done, Open Monitor",
        "ignore Monitor"
      );
      if (proceed !== "Done, Open Monitor") {
        return;
      }
    }
  }

  if (exitCode === undefined) {
    // You can use the word cancel in the confirmation dialog of the flash step - do not add anything.
    return;
  }
  if (exitCode !== 0) {
    // already reported ro error in runFlashCombined/runFlashStm32Bootloader.
    return;
  }

  if (state.target === "FU2") {
    await monitor.open(state);
    return;
  }

  // dynamically run Serial Monitor NATIVE (if open, see serialMonitor/monitor.ts) before opening the
  // miniterm via CMake task below - both are 2 iterated reads that can hold the same serial ports
  // (true bug has gap: "Could not exclusively lock port" when both open the same port). monitor.ts
  // automatically dynamically runs the old version of itself NO before opening each file, but does
  // does not know about the other monitor, so close it explicitly before starting this
  // command. The previous task (Serial Monitor, opened by its command)
  // native) is not processed (needs to change vscode.TaskExecution separately, not yet) - it gap
  // than this microtask using terminal panel viewing.
  await monitor.close();

  const monitorExitCode = await runCMakeTarget({
    taskName: `NEON-MDF: build + flash + log (${state.target})`,
    preset: state.target,
    cmakeTarget: "serial_monitor",
    cwd: folder.uri,
    env: { NEON_MDF_SERIAL_PORT: state.port },
    background: true,
  });

  if (monitorExitCode !== 0) {
    vscode.window.showErrorMessage(`NEON-MDF: Opening serial monitor failed (target ${state.target}) - see terminal.`);
  }
}
