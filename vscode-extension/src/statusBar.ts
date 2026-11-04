// statusBar.ts - 3 StatusBarItem (target/transport/port), chi la UI phan
// anh NeonMdfState - khong tu giu state rieng, luon refresh() theo
// onDidChange de tranh lech voi Device Explorer/webview sau nay.
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
    this.targetItem.text = `$(circuit-board) ${this.state.target || "chon target"}`;
    this.targetItem.tooltip = "NEON-MDF: Chon Target";

    this.transportItem.text =
      this.state.transport === "jtag" ? "$(circuit-board) JTAG" : "$(plug) UART";
    this.transportItem.tooltip =
      "NEON-MDF: Chien luoc Flash (JTAG = qua SWD 1 phien, nhanh; UART = qua System Bootloader, can thao tac tay)";

    this.portItem.text = `$(radio-tower) ${this.state.port ?? "chua chon cong"}`;
    this.portItem.tooltip = "NEON-MDF: Chon Cong Serial";
  }

  dispose(): void {
    this.subscription.dispose();
    this.targetItem.dispose();
    this.transportItem.dispose();
    this.portItem.dispose();
  }
}
