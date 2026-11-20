import { basename } from "node:path";
import * as vscode from "vscode";
import { findApplications, resolveApplication } from "./application";
import { getConfiguredSdkPath } from "./sdkPath";
import { NeonMdfState } from "./state";

type Group = "Project" | "Device" | "Actions";
const groups: Group[] = ["Project", "Device", "Actions"];

class Section extends vscode.TreeItem {
  constructor(readonly group: Group) {
    super(group, vscode.TreeItemCollapsibleState.Expanded);
    this.id = group;
  }
}

function row(label: string, icon: string, command?: string, description?: string): vscode.TreeItem {
  const item = new vscode.TreeItem(label);
  item.iconPath = new vscode.ThemeIcon(icon);
  item.description = description;
  if (command) {
    item.command = { command: `neonMdf.${command}`, title: label };
  }
  return item;
}

function baudRow(label: string, baud: number, setting: string): vscode.TreeItem {
  const item = row(label, "settings-gear", undefined, String(baud));
  item.command = {
    command: "workbench.action.openSettings",
    title: label,
    arguments: [`@id:neonMdf.${setting}`],
  };
  return item;
}

export function registerSidebar(context: vscode.ExtensionContext, state: NeonMdfState): void {
  const changed = new vscode.EventEmitter<void>();
  context.subscriptions.push(
    changed,
    state.onDidChange(() => changed.fire()),
    vscode.window.onDidChangeActiveTextEditor(() => changed.fire()),
    vscode.workspace.onDidChangeWorkspaceFolders(() => changed.fire()),
    vscode.workspace.onDidChangeConfiguration((event) => {
      if (event.affectsConfiguration("neonMdf")) {
        changed.fire();
      }
    }),
    vscode.window.createTreeView("neonMdf.controlPanel", {
      showCollapseAll: false,
      treeDataProvider: {
        onDidChangeTreeData: changed.event,
        getTreeItem: (item: vscode.TreeItem) => item,
        getChildren: async (item?: vscode.TreeItem) => {
          if (!item) {
            return groups.map((group) => new Section(group));
          }
          if (!(item instanceof Section)) {
            return [];
          }
          switch (item.group) {
            case "Project": {
              const app = resolveApplication(await findApplications(), state.selectedApplication);
              const application = row("Application", "folder-opened", "selectApp",
                                      app ? basename(app.uri.fsPath) : "Choose application");
              application.tooltip = app?.uri.fsPath ?? "Select a GT32U2086 application";
              const sdkPath = getConfiguredSdkPath(app?.uri);
              const sdk = row("SDK Path", "settings-gear", "selectSdkPath", sdkPath ? basename(sdkPath) : "Auto");
              sdk.tooltip = sdkPath ?? "Use the SDK linked by the application";
              return [
                application,
                row("Target", "circuit-board", "selectTarget", state.target),
                sdk,
                row("Create New App…", "add", "createApp"),
              ];
            }
            case "Device":
              return [
                row("Transport", "plug", "selectTransport", state.transport.toUpperCase()),
                row("Serial Port", "plug", "selectPort", state.port ?? "Choose port"),
                baudRow("Flash Baud", state.flashBaudRate, "gt32u2086FlashBaudRate"),
                baudRow("Monitor Baud", state.baudRate, "baudRate"),
                row("UART Upload", "info", undefined, state.flashStatus),
              ];
            case "Actions": {
              const flash = row("Flash via UART", "arrow-down", "flash");
              flash.tooltip = "Set boot mode 100. Press RESET when the terminal reports that UART is ready.";
              return [
                row("Build", "tools", "build"),
                flash,
                row("Build + Flash + Log", "run-all", "buildFlashMonitor"),
                row("Serial Monitor", "terminal", "openMonitor"),
                row("Menuconfig", "settings-gear", "menuconfig"),
                row("Debug", "debug-alt", "debug"),
              ];
            }
          }
        },
      },
    }),
  );
}
