// selectSdkPath.ts - same as "ESP-IDF: Configure ESP-IDF Extension" command (select idf.espIdfPath):
// only have one directory, validate no like a real neon-mdf checkout, save neonMdf.sdkPath (Global)
// settings. "reset to automatic" erase setting, return to old swap path conventions.
import * as vscode from "vscode";
import { getConfiguredSdkPath, setConfiguredSdkPath, looksLikeSdkRoot } from "../sdkPath";

const BROWSE_LABEL = "$(folder-opened) Browse my neon-mdf directory...";
const RESET_LABEL = "$(discard) reset to automatic";

export async function selectSdkPathCommand(): Promise<void> {
  const current = getConfiguredSdkPath();

  const items: vscode.QuickPickItem[] = [
    { label: BROWSE_LABEL, description: "select 1 real SDK checkout" },
    {
      label: RESET_LABEL,
      description: "select erase, reuse the modified dan convention (../../neon-mdf)",
    },
  ];

  const placeHolder = current
    ? `in use: ${current}`
    : "using default (automatically inferred from app value) - select to change";

  const picked = await vscode.window.showQuickPick(items, { placeHolder });
  if (!picked) {
    return;
  }

  if (picked.label === RESET_LABEL) {
    await setConfiguredSdkPath(undefined);
    vscode.window.showInformationMessage("NEON-MDF: reset to default SDK path (automatic).");
    return;
  }

  const chosen = await vscode.window.showOpenDialog({
    canSelectFolders: true,
    canSelectFiles: false,
    canSelectMany: false,
    openLabel: "Select SDK Path",
    defaultUri: current ? vscode.Uri.file(current) : undefined,
  });
  if (!chosen || chosen.length === 0) {
    return;
  }

  const dir = chosen[0];
  if (!(await looksLikeSdkRoot(dir))) {
    const proceed = await vscode.window.showWarningMessage(
      `NEON-MDF: ${dir.fsPath} is not the same as a neon-mdf SDK checkout (no tools/cmake/project.cmake seen) - still use this dan path?`,
      "still use",
      "deactivate"
    );
    if (proceed !== "still use") {
      return;
    }
  }

  await setConfiguredSdkPath(dir.fsPath);
  vscode.window.showInformationMessage(`NEON-MDF: selected SDK path: ${dir.fsPath}`);
}
