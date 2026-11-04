// extension.ts - activate()/deactivate(), wiring only (khong chua logic
// nghiep vu o day - xem cac module rieng trong src/*).
import * as vscode from "vscode";
import { NeonMdfState } from "./state";
import { NeonMdfStatusBar } from "./statusBar";
import { selectTargetCommand } from "./commands/selectTarget";
import { selectTransportCommand } from "./commands/selectTransport";
import { selectPortCommand } from "./commands/selectPort";
import { buildCommand } from "./commands/build";
import { flashCommand } from "./commands/flash";
import { buildFlashMonitorCommand } from "./commands/buildFlashMonitor";
import { debugCommand } from "./commands/debug";
import { menuconfigCommand } from "./commands/menuconfig";
import { createAppCommand } from "./commands/createApp";
import { selectSdkPathCommand } from "./commands/selectSdkPath";
import { openMonitorCommand } from "./commands/openMonitor";
import { NeonMdfSerialMonitor } from "./serialMonitor/monitor";

export function activate(context: vscode.ExtensionContext) {
  console.log("[NEON-MDF Tools] activated");

  const state = new NeonMdfState(context);
  const statusBar = new NeonMdfStatusBar(state);
  const monitor = new NeonMdfSerialMonitor();

  context.subscriptions.push(
    state,
    statusBar,
    monitor,
    vscode.commands.registerCommand("neonMdf.selectTarget", () => selectTargetCommand(state)),
    vscode.commands.registerCommand("neonMdf.selectTransport", () => selectTransportCommand(state)),
    vscode.commands.registerCommand("neonMdf.selectPort", () => selectPortCommand(state)),
    vscode.commands.registerCommand("neonMdf.build", () => buildCommand(state)),
    vscode.commands.registerCommand("neonMdf.flash", () => flashCommand(state, monitor)),
    vscode.commands.registerCommand("neonMdf.buildFlashMonitor", () => buildFlashMonitorCommand(state, monitor)),
    vscode.commands.registerCommand("neonMdf.debug", () => debugCommand(state)),
    vscode.commands.registerCommand("neonMdf.menuconfig", () => menuconfigCommand(state)),
    vscode.commands.registerCommand("neonMdf.createApp", () => createAppCommand()),
    vscode.commands.registerCommand("neonMdf.selectSdkPath", () => selectSdkPathCommand()),
    vscode.commands.registerCommand("neonMdf.openMonitor", () => openMonitorCommand(state, monitor))
  );
}

export function deactivate() {
  // khong co gi can don dep o day - moi thu da dang ky qua context.subscriptions
}
