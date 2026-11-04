// detectPort.ts - automatically that port is running bootloader (see serialMonitor/portProbe.ts). You
// Reset or power-cycle the board immediately before this command; the bootloader accepts SYNC only
// in 1 of the short numbers after reset.
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { detectBootloaderPort } from "../serialMonitor/portProbe";

export async function detectPortCommand(state: NeonMdfState): Promise<void> {
  await vscode.window.withProgress(
    {
      location: vscode.ProgressLocation.Notification,
      title: "NEON-MDF: Probing serial ports - reset or power-cycle the board now...",
    },
    async () => {
      const found = await detectBootloaderPort();
      if (!found) {
        vscode.window.showWarningMessage(
          "NEON-MDF: No bootloader responded within the SYNC window (~9.5s). Reset or power-cycle the board while this command runs, or select a port manually."
        );
        return;
      }
      await state.setPort(found);
      vscode.window.showInformationMessage(`NEON-MDF: found bootloader on ${found} port.`);
    }
  );
}
