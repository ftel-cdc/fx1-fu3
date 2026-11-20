// Create an application from the SDK's bare-metal or FreeRTOS template.
// Templates are read from the selected SDK rather than bundled in the extension.
// Replace __APP_NAME__ and __NEON_MDF_RELATIVE_PATH__ using the chosen destination.
import * as vscode from "vscode";
import { getConfiguredSdkPath, resolveSdkPathUri, relativePathToSdk, looksLikeSdkRoot } from "../sdkPath";

const APP_NAME_PATTERN = /^[A-Za-z_][A-Za-z0-9_]*$/;

async function pathExists(uri: vscode.Uri): Promise<boolean> {
  try {
    await vscode.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}

/**
 * Use examples/ when the workspace is the repository root with a valid SDK.
 * Otherwise use the workspace's parent, as when an example application is open.
 * Verify the SDK marker to avoid mistaking an unrelated examples/ directory for
 * the repository layout.
 */
async function resolveExamplesDir(openedFolder: vscode.Uri): Promise<vscode.Uri> {
  const childExamples = vscode.Uri.joinPath(openedFolder, "examples");
  const childSdk = vscode.Uri.joinPath(openedFolder, "neon-mdf");
  if ((await pathExists(childExamples)) && (await looksLikeSdkRoot(childSdk))) {
    return childExamples;
  }
  return vscode.Uri.joinPath(openedFolder, "..");
}

/**
 * Copy the template tree and replace tokens in file contents, not filenames.
 * All current template files are text; binary assets would need separate handling.
 */
async function copyTemplateDir(src: vscode.Uri, dest: vscode.Uri, tokens: Record<string, string>): Promise<void> {
  await vscode.workspace.fs.createDirectory(dest);
  const entries = await vscode.workspace.fs.readDirectory(src);
  for (const [name, type] of entries) {
    const srcChild = vscode.Uri.joinPath(src, name);
    const destChild = vscode.Uri.joinPath(dest, name);
    if (type === vscode.FileType.Directory) {
      await copyTemplateDir(srcChild, destChild, tokens);
    } else {
      const bytes = await vscode.workspace.fs.readFile(srcChild);
      let text = Buffer.from(bytes).toString("utf8");
      for (const [token, value] of Object.entries(tokens)) {
        text = text.split(token).join(value);
      }
      await vscode.workspace.fs.writeFile(destChild, Buffer.from(text, "utf8"));
    }
  }
}

export async function createAppCommand(): Promise<void> {
  const active = vscode.window.activeTextEditor?.document.uri;
  const folder = (active && vscode.workspace.getWorkspaceFolder(active)) ?? vscode.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode.window.showWarningMessage(
      "NEON-MDF: No workspace folder is open. Open the repository root (neon_mdf/) or an application such as examples/Blinky/."
    );
    return;
  }

  const appName = await vscode.window.showInputBox({
    prompt: "App name, used for the folder and CMake target: letters, digits and underscores; must not start with a digit",
    placeHolder: "e.g. my_app",
    validateInput: (value) => {
      if (!value) {
        return "Enter an application name.";
      }
      if (!APP_NAME_PATTERN.test(value)) {
        return "Use letters, digits and underscores; do not start with a digit, for example my_app";
      }
      return undefined;
    },
  });
  if (!appName) {
    return;
  }

  // Templates differ in their source files and scheduler integration.
  // Both provide a minimal UART example without a GPIO/LED demonstration.
  const templateChoice = await vscode.window.showQuickPick(
    [
      {
        label: "$(circuit-board) Bare-metal app",
        description: "Main loop without FreeRTOS",
        templateFolder: "templates",
      },
      {
        label: "$(list-tree) App using FreeRTOS",
        description: "1 task + scheduler, see examples/FreeRTOS_Blinky/ for more detail",
        templateFolder: "templates_freertos",
      },
    ],
    { placeHolder: "What kind of app?" }
  );
  if (!templateChoice) {
    return;
  }

  const examplesDir = await resolveExamplesDir(folder.uri);
  const defaultDest = vscode.Uri.joinPath(examplesDir, appName);

  const picked = await vscode.window.showQuickPick(
    [
      { label: "$(folder) Use suggested location", description: defaultDest.fsPath, dest: defaultDest },
      { label: "$(folder-opened) Select another folder...", description: "Browse for a destination directory", dest: undefined },
    ],
    { placeHolder: "Choose where to create the application" }
  );
  if (!picked) {
    return;
  }

  let destDir = picked.dest;
  if (!destDir) {
    const chosen = await vscode.window.showOpenDialog({
      canSelectFolders: true,
      canSelectFiles: false,
      canSelectMany: false,
      openLabel: "Create app here",
    });
    if (!chosen || chosen.length === 0) {
      return;
    }
    destDir = vscode.Uri.joinPath(chosen[0], appName);
  }

  if (await pathExists(destDir)) {
    vscode.window.showErrorMessage(`NEON-MDF: directory already exists: ${destDir.fsPath}`);
    return;
  }

  // Locate the SDK from the open workspace before computing the new project's relative path.
  const rootSdk = vscode.Uri.joinPath(folder.uri, "neon-mdf");
  const sdkDir = !getConfiguredSdkPath(folder.uri) && await looksLikeSdkRoot(rootSdk)
    ? rootSdk
    : resolveSdkPathUri(folder.uri);
  if (!(await looksLikeSdkRoot(sdkDir))) {
    vscode.window.showErrorMessage(
      `NEON-MDF: No SDK found at ${sdkDir.fsPath}. Run "NEON-MDF: Select SDK Path..." and choose the SDK directory.`
    );
    return;
  }

  const templatesDir = vscode.Uri.joinPath(sdkDir, "tools", templateChoice.templateFolder);
  if (!(await pathExists(templatesDir))) {
    vscode.window.showErrorMessage(`NEON-MDF: template directory not found: ${templatesDir.fsPath}`);
    return;
  }

  const relPath = relativePathToSdk(destDir, sdkDir);

  await copyTemplateDir(templatesDir, destDir, {
    __APP_NAME__: appName,
    __NEON_MDF_RELATIVE_PATH__: relPath,
  });

  const openNow = await vscode.window.showInformationMessage(
    `NEON-MDF: created app "${appName}" at ${destDir.fsPath}`,
    "Open Application"
  );
  if (openNow) {
    await vscode.commands.executeCommand("vscode.openFolder", destDir, { forceNewWindow: true });
  }
}
