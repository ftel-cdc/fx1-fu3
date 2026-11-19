// Three status bar items reflect NeonMdfState (target, transport and port).
// Refresh on onDidChange so all extension views share the same state.
import * as vscode from "vscode";
import { NeonMdfState } from "./state";

export class NeonMdfStatusBar implements vscode.Disposable {
  private readonly targetItem: vscode.StatusBarItem;
  private readonly transportItem: vscode.StatusBarItem;
  private readonly portItem: vscode.StatusBarItem;
  private readonly subscription: vscode.Disposable;

  constructor(private readonly state: NeonMdfState) {
    this.targetItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 100);
    this.targetItem.command = "neonMdf.selectTarget";

    this.transportItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 99);
    this.transportItem.command = "neonMdf.selectTransport";

    this.portItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 98);
    this.portItem.command = "neonMdf.selectPort";

    this.subscription = state.onDidChange(() => this.refresh());
    this.refresh();

    this.targetItem.show();
    this.transportItem.show();
    this.portItem.show();
  }

  private refresh(): void {
    this.targetItem.text = `$(circuit-board) ${this.state.target || "select target"}`;
    this.targetItem.tooltip = "NEON-MDF: Select Target";

    this.transportItem.text = "$(plug) UART";
    this.transportItem.tooltip = "NEON-MDF: ROMCODE UART; select boot mode 100, then RESET when ready";

    this.portItem.text = `$(radio-tower) ${this.state.port ?? "no port selected"}`;
    this.portItem.tooltip = "NEON-MDF: Select Serial Port";
  }

  dispose(): void {
    this.subscription.dispose();
    this.targetItem.dispose();
    this.transportItem.dispose();
    this.portItem.dispose();
  }
}
