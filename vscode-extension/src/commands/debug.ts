import * as vscode from "vscode";
import { Application } from "../application";

export async function debugCommand(app: Application): Promise<void> {
  const configurations = vscode.workspace.getConfiguration("launch", app.folder.uri)
    .get<vscode.DebugConfiguration[]>("configurations", [])
    .filter((config) => config.type === "cortex-debug" && config.request === "attach");
  if (configurations.length === 0) {
    throw new Error("Add a Cortex-Debug attach configuration to .vscode/launch.json. See guide/VSCode.md for an example.");
  }
  if (!vscode.extensions.getExtension("marus25.cortex-debug")) {
    throw new Error("Install Cortex-Debug (marus25.cortex-debug) to attach to the board.");
  }
  const config = configurations.length === 1
    ? configurations[0]
    : (await vscode.window.showQuickPick(
      configurations.map((entry) => ({ label: entry.name, config: entry })),
      { placeHolder: "Select a Cortex-Debug configuration" },
    ))?.config;
  if (config && !(await vscode.debug.startDebugging(app.folder, config.name))) {
    throw new Error("Debug could not start. Check the Cortex-Debug output and the board's JTAG configuration.");
  }
}
