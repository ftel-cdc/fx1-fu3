// Flash dispatches UART by target: FU2 ROMCODE -> flash_uart_fu2;
// STM32 System Bootloader -> flash_uart_stm32. FU2 waits for manual RESET
// after opening UART and does not require OpenOCD. Debugger programming
// uses flash_jtag only on supported targets. Shared functions return the
// actual task exit code so Build + Flash + Log can stop on failure.
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { runCMakeTarget } from "../cmakeRunner";
import { ensureConfigured } from "../configure";
import { resolveSdkPathUri } from "../sdkPath";
import { NeonMdfSerialMonitor } from "../serialMonitor/monitor";

async function fileExists(uri: vscode.Uri): Promise<boolean> {
  try {
    await vscode.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}

// both flash_jtag times flash_uart_stm32 (see cmake/flash_targets.cmake) need openocd .cfg file -
// flash_jtag uses an OpenOCD session; flash_uart_stm32 separately uses OpenOCD to reset through
// SWD by writing AIRCR before starting the UART client. Without this early check, missing .cfg files
// such as CH32V003F4P6 fail later
// straight into error CMake/openocd tho in terminal see because 1 message ro
// explicitly at extension level, as debug.ts already does.
async function checkOpenocdCfg(state: NeonMdfState, folder: vscode.Uri): Promise<boolean> {
  const openocdCfg = vscode.Uri.joinPath(resolveSdkPathUri(folder), "cmake", "openocd", `${state.target}_openocd.cfg`);
  if (!(await fileExists(openocdCfg))) {
    vscode.window.showErrorMessage(`NEON-MDF: could not find openocd cfg file: ${openocdCfg.fsPath}`);
    return false;
  }
  return true;
}

export async function flashCommand(state: NeonMdfState, monitor: NeonMdfSerialMonitor): Promise<void> {
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

  if (state.transport === "jtag") {
    await runFlashCombined(state, folder.uri);
    return;
  }

  await monitor.close();
  await runFlashUart(state, folder.uri);
}

export async function runFlashCombined(state: NeonMdfState, folder: vscode.Uri): Promise<number | undefined> {
  if (state.target === "FU2") {
    vscode.window.showErrorMessage("NEON-MDF: FU2 OpenOCD does not have a flash configuration. select Transport UART to flash via ROMCODE.");
    return undefined;
  }
  if (!(await checkOpenocdCfg(state, folder))) {
    return undefined;
  }

  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: flash (${state.target}, SWD)`,
    preset: state.target,
    cmakeTarget: "flash_jtag",
    cwd: folder,
  });

  if (exitCode !== 0) {
    vscode.window.showErrorMessage(`NEON-MDF: flash via SWD failed (target ${state.target}) - see terminal.`);
  }
  return exitCode;
}

export async function runFlashUart(state: NeonMdfState, folder: vscode.Uri): Promise<number | undefined> {
  if (state.target !== "FU2") {
    return runFlashStm32Bootloader(state, folder);
  }
  if (!state.port) {
    vscode.window.showWarningMessage("NEON-MDF: select the serial port before flashing UART FU2.");
    return undefined;
  }
  if (!Number.isInteger(state.flashBaudRate) || state.flashBaudRate <= 0) {
    vscode.window.showErrorMessage("NEON-MDF: neonMdf.fu2FlashBaudRate must be a ROMCODE matching line integer.");
    return undefined;
  }
  const exitCode = await runCMakeTarget({
    taskName: "NEON-MDF: FU2 ROMCODE UART update",
    preset: state.target,
    cmakeTarget: "flash_uart_fu2",
    cwd: folder,
    env: {
      NEON_MDF_SERIAL_PORT: state.port,
      NEON_MDF_SERIAL_BAUD: String(state.flashBaudRate),
    },
  });
  if (exitCode !== 0 && exitCode !== undefined) {
    vscode.window.showErrorMessage("NEON-MDF: UART FU2 flash failed; see terminal. test boot mode 100, RESET and baud ROMCODE.");
  }
  return exitCode;
}

export async function runFlashStm32Bootloader(state: NeonMdfState, folder: vscode.Uri): Promise<number | undefined> {
  // required to have selected serial port - CMake target flash_uart_stm32 read dia only true via
  // variable $NEON_MDF_SERIAL_PORT (see cmake/flash_targets.cmake), not from derived from
  // state.port if not TRUYEN down through `env` (see below) - missing this step is a real gap bug:
  // Flash command "successful" but write the default hard-code port in CMake cache
  // (/dev/cu.usbmodem102, specific to one development machine) instead of the port selected by the user. It failed silently (no
  // if it is possible to use that port with other bits).
  if (!state.port) {
    vscode.window.showWarningMessage(
      'NEON-MDF: No serial port selected. Use NEON-MDF: Select Serial Port before UART flashing.'
    );
    return undefined;
  }
  if (!(await checkOpenocdCfg(state, folder))) {
    return undefined;
  }

  const proceed = await vscode.window.showInformationMessage(
    "NEON-MDF: UART flashing requires manual preparation: connect PB2 to GND and hold B0 on the board. Reset is automatic through SWD. Keep both in place until flashing finishes.",
    "leveled, continue",
    "deactivate"
  );
  if (proceed !== "leveled, continue") {
    return undefined;
  }

  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: flash (${state.target}, UART System Bootloader)`,
    preset: state.target,
    cmakeTarget: "flash_uart_stm32",
    cwd: folder,
    // Pass the actual user-selected port; omitting this previously caused a
    // bug): flash_uart_stm32 always used the hardcoded CMake-cache default instead of the selected board
    // ports.
    env: { NEON_MDF_SERIAL_PORT: state.port },
  });

  if (exitCode !== 0) {
    vscode.window.showErrorMessage(
      `NEON-MDF: flash via UART System Bootloader failed (target ${state.target}) - see terminal. checked that PB2->GND + keep B0 used for reset.`
    );
  } else {
    vscode.window.showInformationMessage(
      "NEON-MDF: Flashing succeeded. Release B0, disconnect PB2 from GND, then reset to run the app."
    );
  }
  return exitCode;
}
