// selectPort.ts - QuickPick chon cong serial, danh sach THAT tu
// ports.ts (serialport.list()) - luon co lua chon "tu dong do" (xem
// serialMonitor/portProbe.ts, buoc nay yeu cau board vua duoc reset) va
// "nhap tay" phong khi board chua cam vao hoac serialport khong liet ke
// duoc no (vd ST-Link VCP bi OS an di).
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { listPorts } from "../ports";
import { detectPortCommand } from "./detectPort";

const MANUAL_ENTRY = "$(edit) Nhap tay...";
const AUTO_DETECT_ENTRY = "$(zap) Tu dong do (RESET board truoc)...";

export async function selectPortCommand(state: NeonMdfState): Promise<void> {
  let ports: { path: string; manufacturer?: string }[] = [];
  try {
    ports = await listPorts();
  } catch (err) {
    vscode.window.showWarningMessage(
      `NEON-MDF: khong doc duoc danh sach cong serial (${String(err)}) - nhap tay.`
    );
  }

  const items: vscode.QuickPickItem[] = [
    { label: AUTO_DETECT_ENTRY, description: "do cong dang chay bootloader (giao thuc SYNC/READY)" },
    ...ports.map((p) => ({
      label: p.path,
      description: p.manufacturer,
    })),
    { label: MANUAL_ENTRY, description: "go duong dan cong serial" },
  ];

  const picked = await vscode.window.showQuickPick(items, {
    placeHolder: ports.length > 0 ? "Chon cong serial" : "Khong tim thay cong nao - nhap tay hoac tu dong do",
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
      prompt: "Duong dan cong serial (vd /dev/cu.usbmodem102, COM3, /dev/ttyACM0)",
      value: state.port ?? "",
    });
    if (!typed) {
      return;
    }
    port = typed;
  }

  await state.setPort(port);
}
