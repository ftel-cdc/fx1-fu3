import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { listPorts } from "../ports";

const MANUAL_ENTRY = "$(edit) Enter serial port...";

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
    ...ports.map((p) => ({
      label: p.path,
      description: p.manufacturer,
    })),
    { label: MANUAL_ENTRY, description: "type serial port" },
  ];

  const picked = await vscode.window.showQuickPick(items, {
    placeHolder: ports.length > 0 ? "select serial port" : "No ports found; enter the serial port manually",
  });
  if (!picked) {
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
