// selectPort.ts - QuickPick selects the correct serial number from ports.ts (serialport.list()) - this
// option allows you to select "automatically" (see serialMonitor/portProbe.ts, this step is only
// necessary if your board has been reset) and "enter handset" in case the board is not yet
// installed or the serialport is not available (e.g. ST-Link VCP secure OS).
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { listPorts } from "../ports";
import { detectPortCommand } from "./detectPort";

const MANUAL_ENTRY = "$(edit) enter hand...";
const AUTO_DETECT_ENTRY = "$(zap) auto that (RESET board before)...";

export async function selectPortCommand(state: NeonMdfState): Promise<void> {
  let ports: { path: string; manufacturer?: string }[] = [];
  try {
    ports = await listPorts();
  } catch (err) {
    vscode.window.showWarningMessage(
      `NEON-MDF: unable to read serial ports list (${String(err)}) - manual input.`
    );
  }

  const items: vscode.QuickPickItem[] = [
    { label: AUTO_DETECT_ENTRY, description: "that port is running bootloader (real delivery SYNC/READY)" },
    ...ports.map((p) => ({
      label: p.path,
      description: p.manufacturer,
    })),
    { label: MANUAL_ENTRY, description: "type serial port" },
  ];

  const picked = await vscode.window.showQuickPick(items, {
    placeHolder: ports.length > 0 ? "select serial port" : "no knob found - enter that hand or auto",
  });
  if (!picked) {
    return;
  }

  if (picked.label === AUTO_DETECT_ENTRY) {
    await detectPortCommand(state);
    return;
  }

  let port = picked.label;
  if (picked.label === MANUAL_ENTRY) {
    const typed = await vscode.window.showInputBox({
      prompt: "Serial port, e.g. /dev/cu.usbmodem102, COM3 or /dev/ttyACM0",
      value: state.port ?? "",
    });
    if (!typed) {
      return;
    }
    port = typed;
  }

  await state.setPort(port);
}
