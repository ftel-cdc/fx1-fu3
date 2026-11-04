// selectTarget.ts - QuickPick chon target, danh sach lay THAT tu
// CMakePresets.json qua presets.ts (khong hardcode FU2/WEACT_F412).
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { discoverTargets } from "../presets";

export async function selectTargetCommand(state: NeonMdfState): Promise<void> {
  const folder = vscode.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode.window.showWarningMessage("NEON-MDF: chua mo workspace folder nao.");
    return;
  }

  const targets = await discoverTargets(folder.uri);
  if (targets.length === 0) {
    vscode.window.showWarningMessage(
      "NEON-MDF: khong tim thay target nao trong CMakePresets.json cua workspace nay - dam bao dang mo dung 1 thu muc vi du (vd examples/Blinky), khong phai thu muc goc repo."
    );
    return;
  }

  const picked = await vscode.window.showQuickPick(
    targets.map((t) => ({ label: t.displayName, description: t.name })),
    { placeHolder: "Chon target (doc tu CMakePresets.json)" }
  );
  if (!picked) {
    return;
  }
  await state.setTarget(picked.description ?? picked.label);
}
