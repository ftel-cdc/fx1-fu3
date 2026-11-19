import * as vscode from "vscode";
import { NeonMdfState } from "../state";

export async function debugCommand(_state: NeonMdfState): Promise<void> {
  vscode.window.showInformationMessage(
    "NEON-MDF: GT32U2086 JTAG/OpenOCD debugging is not configured. Use UART Flash and Serial Monitor; see guide/Debugging.md in the SDK repository."
  );
}
