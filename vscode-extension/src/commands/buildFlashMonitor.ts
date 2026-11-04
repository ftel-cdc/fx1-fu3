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

  if (!state.port) {
    vscode.window.showWarningMessage(
      'NEON-MDF: chua chon cong serial - can cho ca flash (duong UART) lan serial monitor.'
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
        "NEON-MDF: Da THA nut B0 + RUT day PB2->GND + reset lai chua? App CHUA CHAY cho toi khi lam xong buoc nay.",
        "Da xong, Mo Monitor",
        "Bo qua Monitor"
      );
      if (proceed !== "Da xong, Mo Monitor") {
        return;
      }
    }
  }

  if (exitCode === undefined) {
    // Nguoi dung tu huy o dialog xac nhan cua buoc flash - khong lam gi them.
    return;
  }
  if (exitCode !== 0) {
    // Da bao loi ro trong runFlashCombined/runFlashStm32Bootloader roi.
    return;
  }

  if (state.target === "FU2") {
    await monitor.open(state);
    return;
  }

  // Dong phien Serial Monitor NATIVE (neu dang mo, xem serialMonitor/monitor.ts)
  // TRUOC khi mo miniterm qua CMake task o duoi - ca 2 la 2 co che DOC LAP
  // cung co the giu cung 1 cong serial (bug that da gap: "Could not
  // exclusively lock port" khi ca 2 cung mo 1 cong). monitor.ts da tu dong
  // dong phien cu cua CHINH NO truoc khi mo phien moi, nhung KHONG biet gi
  // ve tien trinh miniterm nen o day phai tu goi close() truoc khi chay
  // lenh nay. Chieu nguoc lai (dang chay serial_monitor task nay ma nguoi
  // dung bam "Serial Monitor" native) KHONG duoc xu ly (can theo doi
  // vscode.TaskExecution rieng, chua lam) - it gap hon vi task nay chiem
  // terminal panel nguoi dung dang xem.
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
    vscode.window.showErrorMessage(`NEON-MDF: mo serial monitor that bai (target ${state.target}) - xem terminal.`);
  }
}
