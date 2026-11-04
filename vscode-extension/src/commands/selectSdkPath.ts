// selectSdkPath.ts - giong lenh "ESP-IDF: Configure ESP-IDF Extension" (chon
// idf.espIdfPath): duyet toi 1 thu muc, validate no giong 1 checkout
// neon-mdf THAT, luu vao setting neonMdf.sdkPath (Global). "Dat lai ve tu
// dong" xoa setting, quay ve quy uoc duong dan tuong doi cu.
import * as vscode from "vscode";
import { getConfiguredSdkPath, setConfiguredSdkPath, looksLikeSdkRoot } from "../sdkPath";

const BROWSE_LABEL = "$(folder-opened) Duyet toi thu muc neon-mdf...";
const RESET_LABEL = "$(discard) Dat lai ve tu dong";

export async function selectSdkPathCommand(): Promise<void> {
  const current = getConfiguredSdkPath();

  const items: vscode.QuickPickItem[] = [
    { label: BROWSE_LABEL, description: "chon 1 checkout SDK that" },
    {
      label: RESET_LABEL,
      description: "xoa lua chon, dung lai quy uoc duong dan tuong doi (../../neon-mdf)",
    },
  ];

  const placeHolder = current
    ? `Dang dung: ${current}`
    : "Dang dung mac dinh (tu dong suy tu vi tri app) - chon de doi";

  const picked = await vscode.window.showQuickPick(items, { placeHolder });
  if (!picked) {
    return;
  }

  if (picked.label === RESET_LABEL) {
    await setConfiguredSdkPath(undefined);
    vscode.window.showInformationMessage("NEON-MDF: da dat lai ve duong dan SDK mac dinh (tu dong).");
    return;
  }

  const chosen = await vscode.window.showOpenDialog({
    canSelectFolders: true,
    canSelectFiles: false,
    canSelectMany: false,
    openLabel: "Chon lam SDK path",
    defaultUri: current ? vscode.Uri.file(current) : undefined,
  });
  if (!chosen || chosen.length === 0) {
    return;
  }

  const dir = chosen[0];
  if (!(await looksLikeSdkRoot(dir))) {
    const proceed = await vscode.window.showWarningMessage(
      `NEON-MDF: ${dir.fsPath} khong giong 1 checkout neon-mdf SDK (khong thay tools/cmake/project.cmake) - van dung duong dan nay?`,
      "Van dung",
      "Huy"
    );
    if (proceed !== "Van dung") {
      return;
    }
  }

  await setConfiguredSdkPath(dir.fsPath);
  vscode.window.showInformationMessage(`NEON-MDF: da chon SDK path: ${dir.fsPath}`);
}
