// selectTarget.ts - QuickPick selects target, real list from CMakePresets.json through presets.ts (not
// hardcode FU2/WEACT_F412).
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { discoverTargets } from "../presets";

export async function selectTargetCommand(state: NeonMdfState): Promise<void> {
  const folder = vscode.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode.window.showWarningMessage("NEON-MDF: no workspace folder opened.");
    return;
  }

  const targets = await discoverTargets(folder.uri);
  if (targets.length === 0) {
    vscode.window.showWarningMessage(
      "NEON-MDF: No targets found in this workspace CMakePresets.json. Open an application folder such as examples/Blinky."
    );
    return;
  }

  const picked = await vscode.window.showQuickPick(
    targets.map((t) => ({ label: t.displayName, description: t.name })),
    { placeHolder: "select target (read from CMakePresets.json)" }
  );
  if (!picked) {
    return;
  }
  await state.setTarget(picked.description ?? picked.label);
}
