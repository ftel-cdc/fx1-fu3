import * as path from "node:path";
import * as vscode from "vscode";

/** Use the same filesystem authority as the app when resolving a host path. */
export function sdkPathUri(anchor: vscode.Uri, sdkPath: string): vscode.Uri {
  return anchor.with({ path: vscode.Uri.file(sdkPath).path, query: "", fragment: "" });
}

export async function looksLikeSdkRoot(dir: vscode.Uri): Promise<boolean> {
  try {
    const marker = await vscode.workspace.fs.stat(vscode.Uri.joinPath(dir, "tools", "cmake", "project.cmake"));
    return (marker.type & vscode.FileType.File) !== 0;
  } catch {
    return false;
  }
}

export async function validateSdkPath(anchor: vscode.Uri, sdkPath?: string): Promise<vscode.Uri | undefined> {
  if (sdkPath && path.isAbsolute(sdkPath)) {
    const uri = sdkPathUri(anchor, sdkPath);
    if (await looksLikeSdkRoot(uri)) {
      return uri;
    }
  }
}

/** An empty workspace override selects Auto even if a legacy setting still exists. */
function configuredSdkPath(anchor: vscode.Uri, override?: string): string {
  return override ?? vscode.workspace.getConfiguration("neonMdf", anchor).get("sdkPath", "").trim();
}

export async function resolveSdkPath(
  anchor: vscode.Uri,
  override?: string,
): Promise<{ uri: vscode.Uri; automatic: boolean } | undefined> {
  const configured = await validateSdkPath(anchor, configuredSdkPath(anchor, override));
  if (configured) {
    return { uri: configured, automatic: false };
  }
  // Support an open SDK repository and a bundled application under examples/.
  for (const parts of [["neon-mdf"], ["..", "..", "neon-mdf"]]) {
    const uri = vscode.Uri.joinPath(anchor, ...parts);
    if (await looksLikeSdkRoot(uri)) {
      return { uri, automatic: true };
    }
  }
  const fromEnv = await validateSdkPath(anchor, process.env.NEON_MDF_PATH);
  if (fromEnv) {
    return { uri: fromEnv, automatic: true };
  }
}

export function relativePathToSdk(fromDir: vscode.Uri, sdkDir: vscode.Uri): string {
  return path.relative(fromDir.fsPath, sdkDir.fsPath).split(path.sep).join("/");
}
