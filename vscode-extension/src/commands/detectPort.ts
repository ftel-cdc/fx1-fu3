// detectPort.ts - tu dong do cong dang chay bootloader (xem
// serialMonitor/portProbe.ts). Nguoi dung phai RESET/cap lai nguon board
// NGAY TRUOC khi chay lenh nay - bootloader chi cho SYNC trong 1 cua so
// ngan sau reset.
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { detectBootloaderPort } from "../serialMonitor/portProbe";

export async function detectPortCommand(state: NeonMdfState): Promise<void> {
  await vscode.window.withProgress(
    {
      location: vscode.ProgressLocation.Notification,
      title: "NEON-MDF: dang do cong serial - RESET/cap lai nguon board NGAY BAY GIO...",
    },
    async () => {
      const found = await detectBootloaderPort();
      if (!found) {
        vscode.window.showWarningMessage(
          "NEON-MDF: khong tim thay cong nao dang chay bootloader trong cua so cho SYNC (~9.5s) - dam bao board vua duoc RESET/cap lai nguon dung luc lenh nay chay, hoac chon cong bang tay."
        );
        return;
      }
      await state.setPort(found);
      vscode.window.showInformationMessage(`NEON-MDF: da tim thay bootloader tren cong ${found}.`);
    }
  );
}
