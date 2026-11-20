// Global SDK path, similar to ESP-IDF idf.espIdfPath. An empty setting uses
// the app-relative default: <app>/../../neon-mdf. A selected path is passed
// through NEON_MDF_PATH so existing and generated apps can use another checkout.
import * as vscode from "vscode";
import * as path from "path";

const SETTING_KEY = "sdkPath";

export function getConfiguredSdkPath(scope?: vscode.Uri): string | undefined {
  const raw = vscode.workspace.getConfiguration("neonMdf", scope).get<string>(SETTING_KEY, "");
  const trimmed = raw?.trim();
  return trimmed ? trimmed : undefined;
}

export async function setConfiguredSdkPath(sdkPath: string | undefined): Promise<void> {
  await vscode.workspace
    .getConfiguration("neonMdf")
    .update(SETTING_KEY, sdkPath ?? "", vscode.ConfigurationTarget.Global);
}

/**
 * Override NEON_MDF_PATH for CMake subprocesses when an SDK path is configured.
 * Leave the environment unchanged otherwise so application defaults apply.
 */
export function sdkPathEnv(scope?: vscode.Uri): Record<string, string> | undefined {
  const configured = getConfiguredSdkPath(scope);
  return configured ? { NEON_MDF_PATH: configured } : undefined;
}

/** Resolve the configured SDK path, or <app>/../../neon-mdf by default. */
export function resolveSdkPathUri(anchorDir: vscode.Uri): vscode.Uri {
  const configured = getConfiguredSdkPath(anchorDir);
  if (configured) {
    return vscode.Uri.file(configured);
  }
  return vscode.Uri.joinPath(anchorDir, "..", "..", "neon-mdf");
}

/** Return the app-to-SDK relative path with POSIX separators for CMake files. */
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

/** Identify an SDK checkout by its tools/cmake/project.cmake marker. */
export async function looksLikeSdkRoot(dir: vscode.Uri): Promise<boolean> {
  return fileExists(vscode.Uri.joinPath(dir, "tools", "cmake", "project.cmake"));
}
