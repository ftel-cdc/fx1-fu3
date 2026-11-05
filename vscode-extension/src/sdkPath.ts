// Global SDK path, similar to ESP-IDF idf.espIdfPath. An empty setting uses
// the app-relative default: <app>/../../neon-mdf. A selected path is passed
// through NEON_MDF_PATH so existing and generated apps can use another checkout.
import * as vscode from "vscode";
import * as path from "path";

const SETTING_KEY = "sdkPath";

export function getConfiguredSdkPath(): string | undefined {
  const raw = vscode.workspace.getConfiguration("neonMdf").get<string>(SETTING_KEY, "");
  const trimmed = raw?.trim();
  return trimmed ? trimmed : undefined;
}

export async function setConfiguredSdkPath(sdkPath: string | undefined): Promise<void> {
  await vscode.workspace
    .getConfiguration("neonMdf")
    .update(SETTING_KEY, sdkPath ?? "", vscode.ConfigurationTarget.Global);
}

/**
 * variable for each plugin field for cmake subprocess if the user has chosen a separate SDK path -
 * undefined if used by default (no thiep required, so that CMakeLists.txt/settings.json of each app
 * is set as before). CMakeLists.txt of each app has `if(NOT DEFINED ENV{NEON_MDF_PATH})` guard so
 * this variable, when present, will overwrite the old swap convention without modifying the file at
 * all - it can be applied to both old apps (Blinky ...) and new ones.
 */
export function sdkPathEnv(): Record<string, string> | undefined {
  const configured = getConfiguredSdkPath();
  return configured ? { NEON_MDF_PATH: configured } : undefined;
}

/**
 * The actual SDK path will be applied to an app directory (anchorDir): the dan path is selected in
 * settings if available, otherwise the old convention (2 levels from anchorDir to neon-mdf/, using
 * `${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf` as CMakeLists.txt calculates - see
 * examples/<App>/CMakeLists.txt).
 */
export function resolveSdkPathUri(anchorDir: vscode.Uri): vscode.Uri {
  const configured = getConfiguredSdkPath();
  if (configured) {
    return vscode.Uri.file(configured);
  }
  return vscode.Uri.joinPath(anchorDir, "..", "..", "neon-mdf");
}

/**
 * POSIX interchange path (available directly in CMakeLists.txt/ CMakePresets.json, OS-independent)
 * from an app directory to the SDK, used by createApp.ts when generating a project; see the previous
 * "../../neon-mdf" equals true value dua on the selected app + SDK).
 */
export function relativePathToSdk(fromDir: vscode.Uri, sdkDir: vscode.Uri): string {
  const rel = path.relative(fromDir.fsPath, sdkDir.fsPath);
  return rel.split(path.sep).join("/");
}

async function fileExists(uri: vscode.Uri): Promise<boolean> {
  try {
    await vscode.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}

/** 1 directory is a real SDK neon-mdf checkout please don't - find file marker tools/cmake/project.cmake. */
export async function looksLikeSdkRoot(dir: vscode.Uri): Promise<boolean> {
  return fileExists(vscode.Uri.joinPath(dir, "tools", "cmake", "project.cmake"));
}
