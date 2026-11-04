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

// Ca flash_jtag LAN flash_uart_stm32 (xem cmake/flash_targets.cmake) deu can
// file openocd .cfg - flash_jtag dung ca phien openocd that su, flash_uart_stm32
// dung openocd rieng cho 1 buoc reset qua SWD (ghi AIRCR) truoc khi giao lai
// cho client UART tu viet. Truoc day KHONG co check nay o day (khac han
// debug.ts co san) - thieu file .cfg (vd CH32V003F4P6, chua wire that) se
// roi thang vao loi CMake/openocd tho trong terminal thay vi 1 thong bao ro
// rang cap extension nhu debug.ts da lam.
async function checkOpenocdCfg(state: NeonMdfState, folder: vscode.Uri): Promise<boolean> {
  const openocdCfg = vscode.Uri.joinPath(resolveSdkPathUri(folder), "cmake", "openocd", `${state.target}_openocd.cfg`);
  if (!(await fileExists(openocdCfg))) {
    vscode.window.showErrorMessage(`NEON-MDF: khong tim thay file openocd cfg: ${openocdCfg.fsPath}`);
    return false;
  }
  return true;
}

export async function flashCommand(state: NeonMdfState, monitor: NeonMdfSerialMonitor): Promise<void> {
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

  if (state.transport === "jtag") {
    await runFlashCombined(state, folder.uri);
    return;
  }

  await monitor.close();
  await runFlashUart(state, folder.uri);
}

export async function runFlashCombined(state: NeonMdfState, folder: vscode.Uri): Promise<number | undefined> {
  if (state.target === "FU2") {
    vscode.window.showErrorMessage("NEON-MDF: FU2 OpenOCD chua co cau hinh nap. Hay chon Transport UART de nap qua ROMCODE.");
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
    vscode.window.showErrorMessage(`NEON-MDF: flash qua SWD that bai (target ${state.target}) - xem terminal.`);
  }
  return exitCode;
}

export async function runFlashUart(state: NeonMdfState, folder: vscode.Uri): Promise<number | undefined> {
  if (state.target !== "FU2") {
    return runFlashStm32Bootloader(state, folder);
  }
  if (!state.port) {
    vscode.window.showWarningMessage("NEON-MDF: Chon cong serial truoc khi nap UART FU2.");
    return undefined;
  }
  if (!Number.isInteger(state.flashBaudRate) || state.flashBaudRate <= 0) {
    vscode.window.showErrorMessage("NEON-MDF: neonMdf.fu2FlashBaudRate phai la so nguyen duong khop ROMCODE.");
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
    vscode.window.showErrorMessage("NEON-MDF: Nap UART FU2 that bai; xem terminal. Kiem tra boot mode 100, RESET va baud ROMCODE.");
  }
  return exitCode;
}

export async function runFlashStm32Bootloader(state: NeonMdfState, folder: vscode.Uri): Promise<number | undefined> {
  // BAT BUOC co cong serial da chon - target CMake flash_uart_stm32 doc dia
  // chi that qua bien moi truong $NEON_MDF_SERIAL_PORT (xem
  // cmake/flash_targets.cmake), KHONG tu suy ra tu state.port neu khong
  // duoc TRUYEN xuong qua `env` (xem duoi) - thieu buoc nay la bug that da
  // gap: lenh Flash "thanh cong" nhung ghi vao cong MAC DINH hard-code
  // trong CMake cache (/dev/cu.usbmodem102, dac thu 1 may dev) thay vi cong
  // nguoi dung vua chon, hoan toan im lang (khong bao loi neu tinh co may
  // co dung cong do cam thiet bi khac).
  if (!state.port) {
    vscode.window.showWarningMessage(
      'NEON-MDF: chua chon cong serial - bam "NEON-MDF: Chon Cong Serial" truoc khi flash qua UART.'
    );
    return undefined;
  }
  if (!(await checkOpenocdCfg(state, folder))) {
    return undefined;
  }

  const proceed = await vscode.window.showInformationMessage(
    "NEON-MDF: Flash qua UART can THAO TAC TAY - Noi day PB2 -> GND + GIU nut B0 tren board (KHONG can bam Reset - tu dong reset qua SWD). Giu nguyen ca 2 cho toi khi thay lenh chay xong.",
    "Da san sang, Tiep tuc",
    "Huy"
  );
  if (proceed !== "Da san sang, Tiep tuc") {
    return undefined;
  }

  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: flash (${state.target}, UART System Bootloader)`,
    preset: state.target,
    cmakeTarget: "flash_uart_stm32",
    cwd: folder,
    // Cong THAT nguoi dung da chon - xem comment o dau ham. Thieu dong nay
    // truoc day (bug that) khien flash_uart_stm32 luon dung gia tri mac
    // dinh hard-code trong CMake cache, khong phai cong that cua board.
    env: { NEON_MDF_SERIAL_PORT: state.port },
  });

  if (exitCode !== 0) {
    vscode.window.showErrorMessage(
      `NEON-MDF: flash qua UART System Bootloader that bai (target ${state.target}) - xem terminal. Kiem tra da noi PB2->GND + giu B0 dung luc reset chua.`
    );
  } else {
    vscode.window.showInformationMessage(
      "NEON-MDF: Flash thanh cong - nho THA nut B0 + RUT day PB2->GND, roi reset lai de chay app that."
    );
  }
  return exitCode;
}
