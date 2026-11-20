// Validate and save the SDK directory in the global neonMdf.sdkPath setting.
// Resetting the setting restores the default app-relative SDK path.
import * as vscode from "vscode";
import { getConfiguredSdkPath, setConfiguredSdkPath, looksLikeSdkRoot } from "../sdkPath";

const BROWSE_LABEL = "$(folder-opened) Browse for the SDK...";
const RESET_LABEL = "$(discard) Reset to automatic";

export async function selectSdkPathCommand(): Promise<void> {
  const current = getConfiguredSdkPath();

  const items: vscode.QuickPickItem[] = [
    { label: BROWSE_LABEL, description: "Select a NEON-MDF SDK checkout" },
    {
      label: RESET_LABEL,
      description: "Clear the setting and use ../../neon-mdf relative to the application",
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
      `NEON-MDF: ${dir.fsPath} does not contain tools/cmake/project.cmake. Use this SDK path anyway?`,
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
