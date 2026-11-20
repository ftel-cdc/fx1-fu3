import * as path from "node:path";
import * as vscode from "vscode";
import { discoverTargets } from "./presets";
import type { NeonMdfState } from "./state";

export interface Application {
  uri: vscode.Uri;
  folder: vscode.WorkspaceFolder;
}

async function discoverApps(folder: vscode.WorkspaceFolder): Promise<Application[]> {
  const pattern = new vscode.RelativePattern(folder, "{CMakePresets.json,examples/*/CMakePresets.json}");
  const presets = await vscode.workspace.findFiles(pattern);
  const apps = await Promise.all(presets.map(async (preset) => {
    const uri = vscode.Uri.joinPath(preset, "..");
    try {
      const cmake = await vscode.workspace.fs.readFile(vscode.Uri.joinPath(uri, "CMakeLists.txt"));
      if (Buffer.from(cmake).toString("utf8").includes("tools/cmake/project.cmake") &&
          (await discoverTargets(uri)).some((target) => target.name === "GT32U2086")) {
        return { uri, folder };
      }
    } catch {
      // A directory may have been removed while discovery was running.
    }
    return undefined;
  }));
  return apps.filter((app): app is Application => app !== undefined);
}

export async function findApplications(): Promise<Application[]> {
  const apps = (await Promise.all((vscode.workspace.workspaceFolders ?? []).map(discoverApps))).flat();
  return [...new Map(apps.map((app) => [app.uri.toString(), app])).values()];
}

export function resolveApplication(apps: Application[], selectedUri?: string): Application | undefined {
  const selected = apps.find((app) => app.uri.toString() === selectedUri);
  if (selected) {
    return selected;
  }
  const active = vscode.window.activeTextEditor?.document.uri;
  const activeApp = active && apps.find((app) => {
    if (app.uri.scheme !== active.scheme || app.uri.authority !== active.authority) {
      return false;
    }
    const relative = path.relative(app.uri.fsPath, active.fsPath);
    return relative !== ".." && !relative.startsWith(".." + path.sep) && !path.isAbsolute(relative);
  });
  return activeApp || (apps.length === 1 ? apps[0] : undefined);
}

export async function selectApplication(state: NeonMdfState, prompt = false): Promise<Application | undefined> {
  const apps = await findApplications();
  if (apps.length === 0) {
    vscode.window.showWarningMessage("NEON-MDF: Open an application folder or the NEON-MDF repository.");
    return;
  }
  const current = resolveApplication(apps, state.selectedApplication);
  if (!prompt && current) {
    return current;
  }
  const choices = apps.map((app) => ({
    label: path.basename(app.uri.fsPath),
    description: vscode.workspace.asRelativePath(app.uri, true),
    app,
  })).sort((a, b) => a.label.localeCompare(b.label));
  const choice = await vscode.window.showQuickPick(choices, { placeHolder: "Select a GT32U2086 application" });
  if (choice) {
    await state.setApplication(choice.app.uri);
    return choice.app;
  }
}
