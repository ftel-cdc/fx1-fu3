import * as vscode from "vscode";
import { findApplications, resolveApplication } from "../application";
import { looksLikeSdkRoot, resolveSdkPath } from "../sdkPath";
import { NeonMdfState } from "../state";

export async function selectSdkPathCommand(state: NeonMdfState): Promise<void> {
  const app = resolveApplication(await findApplications(), state.selectedApplication);
  const anchor = app?.uri ?? vscode.workspace.workspaceFolders?.[0]?.uri;
  if (!anchor) {
    vscode.window.showWarningMessage("NEON-MDF: Open a workspace before selecting its SDK.");
    return;
  }

  const current = await resolveSdkPath(anchor, state.sdkPathOverride);
  const items = [
    { label: "$(folder-opened) Browse for the SDK...", value: "browse" },
    { label: "$(discard) Reset to automatic", value: "auto" },
  ];
  const picked = await vscode.window.showQuickPick(items, {
    placeHolder: current ? `Current SDK: ${current.uri.fsPath}` : "Choose a NEON-MDF SDK directory",
  });
  if (!picked) {
    return;
  }
  if (picked.value === "auto") {
    await state.setSdkPath("");
    return;
  }

  const selected = await vscode.window.showOpenDialog({
    canSelectFolders: true,
    canSelectFiles: false,
    canSelectMany: false,
    openLabel: "Select SDK Path",
    defaultUri: current?.uri ?? anchor,
  });
  const dir = selected?.[0];
  if (!dir) {
    return;
  }
  if (dir.scheme !== anchor.scheme || dir.authority !== anchor.authority || !(await looksLikeSdkRoot(dir))) {
    vscode.window.showErrorMessage("NEON-MDF: Choose a directory on the workspace host that contains tools/cmake/project.cmake.");
    return;
  }
  await state.setSdkPath(dir.fsPath);
}
