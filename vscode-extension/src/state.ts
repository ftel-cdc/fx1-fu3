import * as vscode from "vscode";

const PORT_STATE_KEY = "neonMdf.selectedPort";
const SDK_PATH_STATE_KEY = "neonMdf.workspaceSdkPath";
const APPLICATION_STATE_KEY = "neonMdf.selectedApplication";

export class NeonMdfState implements vscode.Disposable {
  private readonly change = new vscode.EventEmitter<void>();
  readonly onDidChange = this.change.event;
  private uploadStatus = "Idle";

  constructor(private readonly context: vscode.ExtensionContext) {}

  get selectedApplication(): string | undefined {
    return this.context.workspaceState.get<string>(APPLICATION_STATE_KEY);
  }

  async setApplication(uri: vscode.Uri): Promise<void> {
    await this.context.workspaceState.update(APPLICATION_STATE_KEY, uri.toString());
    this.change.fire();
  }

  get sdkPathOverride(): string | undefined {
    return this.context.workspaceState.get<string>(SDK_PATH_STATE_KEY);
  }

  async setSdkPath(path: string): Promise<void> {
    await this.context.workspaceState.update(SDK_PATH_STATE_KEY, path);
    this.change.fire();
  }

  get flashStatus(): string {
    return this.uploadStatus;
  }

  setFlashStatus(status: string): void {
    this.uploadStatus = status;
    this.change.fire();
  }

  get target(): string {
    return "GT32U2086";
  }

  async setTarget(target: string): Promise<void> {
    await vscode.workspace.getConfiguration("neonMdf").update("target", target, vscode.ConfigurationTarget.Workspace);
    this.change.fire();
  }

  get transport(): "uart" {
    return "uart";
  }

  async setTransport(transport: "uart"): Promise<void> {
    await vscode.workspace.getConfiguration("neonMdf").update("transport", transport, vscode.ConfigurationTarget.Workspace);
    this.change.fire();
  }

  get baudRate(): number {
    return vscode.workspace.getConfiguration("neonMdf").get("baudRate", 19200);
  }

  get flashBaudRate(): number {
    return vscode.workspace.getConfiguration("neonMdf").get("gt32u2086FlashBaudRate", 19200);
  }

  get port(): string | undefined {
    return this.context.workspaceState.get<string>(PORT_STATE_KEY);
  }

  async setPort(port: string): Promise<void> {
    await this.context.workspaceState.update(PORT_STATE_KEY, port);
    this.change.fire();
  }

  dispose(): void {
    this.change.dispose();
  }
}
