// state.ts - noi duy nhat luu trang thai lua chon (target/transport/port).
// statusBar.ts va cac command deu doc/ghi qua day va lang nghe onDidChange -
// tranh moi UI surface tu giu 1 ban sao trang thai rieng roi lech nhau.
//
// target/transport = vscode.workspace settings ("neonMdf.target"/"neonMdf.transport",
// xem package.json contributes.configuration) - y nghia du an, dang commit
// duoc vao .vscode/settings.json cho ca team, giong cach cmake.sourceDirectory
// da lam trong cac vi du hien co.
// port = workspaceState (Memento) - dac thu MAY (cong USB nao tren MAY nay),
// KHONG nen commit chung voi settings.json vi may khac se co ten cong khac.
import * as vscode from "vscode";

export type Transport = "uart" | "jtag";

const PORT_STATE_KEY = "neonMdf.selectedPort";

export class NeonMdfState implements vscode.Disposable {
  private readonly _onDidChange = new vscode.EventEmitter<void>();
  readonly onDidChange = this._onDidChange.event;

  constructor(private readonly context: vscode.ExtensionContext) {}

  private get config(): vscode.WorkspaceConfiguration {
    return vscode.workspace.getConfiguration("neonMdf");
  }

  get target(): string {
    return this.config.get<string>("target", "");
  }

  async setTarget(target: string): Promise<void> {
    await this.config.update("target", target, vscode.ConfigurationTarget.Workspace);
    this._onDidChange.fire();
  }

  get transport(): Transport {
    return this.config.get<Transport>("transport", "uart");
  }

  async setTransport(transport: Transport): Promise<void> {
    await this.config.update("transport", transport, vscode.ConfigurationTarget.Workspace);
    this._onDidChange.fire();
  }

  get baudRate(): number {
    const values = this.config.inspect<number>("baudRate");
    return values?.workspaceFolderValue ?? values?.workspaceValue ?? values?.globalValue ??
      (this.target === "FU2" ? 19200 : 115200);
  }

  get flashBaudRate(): number {
    return this.config.get<number>("fu2FlashBaudRate", 19200);
  }

  get port(): string | undefined {
    return this.context.workspaceState.get<string>(PORT_STATE_KEY);
  }

  async setPort(port: string | undefined): Promise<void> {
    await this.context.workspaceState.update(PORT_STATE_KEY, port);
    this._onDidChange.fire();
  }

  dispose(): void {
    this._onDidChange.dispose();
  }
}
