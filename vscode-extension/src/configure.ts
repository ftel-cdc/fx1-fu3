// Configure a GT32U2086 application once before build/flash/menuconfig.
import * as vscode from "vscode";
import { runCMakeConfigure } from "./cmakeRunner";

export async function ensureConfigured(cwd: vscode.Uri, preset: string): Promise<boolean> {
  if (preset !== "GT32U2086") {
    vscode.window.showErrorMessage("NEON-MDF: Select target GT32U2086.");
    return false;
  }
  try {
    await vscode.workspace.fs.stat(vscode.Uri.joinPath(cwd, "build", preset, "CMakeCache.txt"));
    return true;
  } catch {
    return (await runCMakeConfigure(preset, cwd)) === 0;
  }
}
