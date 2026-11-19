import * as vscode from "vscode";
import { NeonMdfState } from "../state";

export async function selectTransportCommand(state: NeonMdfState): Promise<void> {
  await state.setTransport("uart");
  vscode.window.showInformationMessage(
    "NEON-MDF: GT32U2086 uses ROMCODE UART. Select boot mode 100 and press RESET when the loader is ready."
  );
}
