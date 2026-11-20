import * as vscode from "vscode";
import { NeonMdfState } from "./state";

export function registerStatusBar(context: vscode.ExtensionContext, state: NeonMdfState): void {
  const actions = [
    ["selectPort", "$(plug) Select Port", "Select the GT32U2086 UART port"],
    ["openMonitor", "$(terminal) Monitor", "Open the interactive UART monitor"],
    ["build", "$(tools) Build", "Build the GT32U2086 application"],
    ["flash", "$(arrow-down) Flash", "Program the firmware through ROMCODE UART"],
    ["menuconfig", "$(settings-gear) Menuconfig", "Configure application and driver options"],
    ["debug", "$(debug-alt) Debug", "Attach using the workspace's Cortex-Debug configuration"],
    ["selectTarget", "$(circuit-board) GT32U2086", "Select the target preset"],
    ["selectTransport", "$(plug) UART", "Select the programming transport"],
  ];
  const items = actions.map(([name, text, tooltip], index) => {
    const item = vscode.window.createStatusBarItem(`neonMdf.${name}`, vscode.StatusBarAlignment.Left, 100 - index);
    item.command = `neonMdf.${name}`;
    item.text = text;
    item.tooltip = tooltip;
    item.show();
    return item;
  });
  const updateSelections = () => {
    items[0].text = state.port ? `$(plug) ${state.port}` : "$(plug) Select Port";
    items[6].text = `$(circuit-board) ${state.target}`;
    items[7].text = `$(plug) ${state.transport.toUpperCase()}`;
  };
  context.subscriptions.push(...items, state.onDidChange(updateSelections));
  updateSelections();
}
