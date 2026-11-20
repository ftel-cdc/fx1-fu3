import * as vscode from "vscode";
import { selectApplication } from "../application";
import { NeonMdfState } from "../state";
import { discoverTargets } from "../presets";

export async function selectTargetCommand(state: NeonMdfState): Promise<void> {
  const app = await selectApplication();
  if (!app) {
    return;
  }

  const targets = (await discoverTargets(app.uri)).filter((target) => target.name === "GT32U2086");
  if (targets.length === 0) {
    vscode.window.showWarningMessage(
      "NEON-MDF: This application has no GT32U2086 CMake preset."
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
