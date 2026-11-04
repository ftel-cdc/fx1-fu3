// createApp.ts implements the New App wizard: choose bare-metal
// please FreeRTOS first, then copy <SDK>/tools/templates/ or
// <SDK>/tools/templates_freertos/ (2 real sources in repo, no bundle and extension - edit template
// without rebuilding or repackaging the extension) to a destination directory. Replace the two tokens inside each file:
// __APP_NAME__ -> user app name choose __NEON_MDF_RELATIVE_PATH__ -> corresponding path from DEST
// DIR to the currently selected SDK (see ../sdkPath.ts) - true on app + SDK value, no assumed
// the old "../../neon-mdf" assumption, which fails when Select Another Folder places an app outside examples/,
// or have chosen separate SDK path via neonMdf.sdkPath). then open in each. both templates were
// built-test for real (both 3 presets FU2/WEACT_F412/CH32V003F4P6, flash+real on WEACT_F412) before
// this feature was written - see CMakeLists.txt/Src/main.c in
// tools/templates/ and tools/templates_freertos/.
//
// templatesDir is derived from sdkDir (not from open workspaceFolder) - it is derived from
// Projects/Templates/ into the SDK on 2026-08-10, like the shared OpenOCD configuration; see
// cmake/openocd/), so you have to change resolveSdkPathUri() of destDir itself to find its use,
// which is how sdkDir/relPath are used below.
import * as vscode from "vscode";
import { resolveSdkPathUri, relativePathToSdk, looksLikeSdkRoot } from "../sdkPath";

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
 * Real examples/ to create new app in which - from receiver fill 2 cases (2026-08-10, real bug fix:
 * previously assumed the workspace was an existing app such as examples/Blinky, then used
 * .. as examples/. Opening the repository root (which contains neon-mdf/ and examples/) instead of an
 * app made that calculation one level too high and selected the wrong destination. Confirmed
 * via user's bug report, not
 * cases):
 *   1. Opening workspace is an app folder (e.g. examples/Blinky) - examples/ is the CHA folder of no (old
 *      behavior, unchanged).
 *   2. If the workspace is the repository root, containing neon-mdf/ and examples/, use its examples/
 * subdirectory. Check this case before the app-directory heuristic for reliable detection
 * markers through looksLikeSdkRoot() on `<folder>/neon-mdf`, not only dua vao `examples/` exist
 * please do not - 1 old app folder turned on periodically can count yes has subdirectory named
 * "examples/" irrelevant). if both 2 do not match (eg user open another safe directory), fallback
 * the previous parent-directory behavior, with a separate SDK-directory warning below when the path is
 * invalid.
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
 * Copy templateDir to destDir and replace the supplied `tokens` inside each file
 * file (not the file name - no template files use tokens in the name). assume each template file is
 * text (correct for all current tools/templates/: .c/.h/.json/.cfg/txt) - if later add more
 * template files, this function must be updated accordingly.
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
  const folder = vscode.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode.window.showWarningMessage(
      "NEON-MDF: No workspace folder is open. Open the repository root (neon_mdf/) or an application such as examples/Blinky/."
    );
    return;
  }

  const appName = await vscode.window.showInputBox({
    prompt: "App name, used for the folder and CMake target: letters, digits and underscores; must not start with a digit",
    placeHolder: "vd my_app",
    validateInput: (value) => {
      if (!value) {
        return "app name not in";
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

  // Bare-metal app (superloop) versus FreeRTOS app (scheduler,
  // xTaskCreate/vTaskStartScheduler). The two templates have different source sets (CMakeLists.txt/
  // Src/main.c/Inc/FreeRTOSConfig.h) font not a flag runtime, en
  // FreeRTOS additionally needs include(cmake/freertos.cmake)/neon_mdf_add_freertos()
  // + glue arch-specific (SVC/PendSV/SysTick for WEACT_F412, STK/mtvec for CH32V003F4P6 - see
  // tools/templates_freertos/Src/main.c) cannot "off" space with #ifdef in a generic template. See
  // tools/templates/ (bare-
  // metal) and tools/templates_freertos/ (FreeRTOS) are both minimal templates
  // are both minimal proof-of-life templates without GPIO/LED demos; they differ in scheduler support
  // not.
  const templateChoice = await vscode.window.showQuickPick(
    [
      {
        label: "$(circuit-board) Bare-metal app",
        description: "superloop don space, no FreeRTOS",
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
      { label: "$(folder) created in examples/ (recommended)", description: defaultDest.fsPath, dest: defaultDest },
      { label: "$(folder-opened) select different folder...", description: "duyet directory", dest: undefined },
    ],
    { placeHolder: "create app at the top?" }
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

  // SDK path true on destDir (not open workspaceFolder) - matches how CMakeLists.txt of new app will be
  // magnetic (${CMAKE_CURRENT_SOURCE_DIR} = destDir for real build). use setting neonMdf.sdkPath if
  // user selected, otherwise old convention (2 level up to neon-mdf/) - see sdkPath.ts.
  const sdkDir = resolveSdkPathUri(destDir);
  if (!(await looksLikeSdkRoot(sdkDir))) {
    const proceed = await vscode.window.showWarningMessage(
      `NEON-MDF: can't find neon-mdf SDK at ${sdkDir.fsPath} (this directory is not the same as a real SDK checkout). if the app is creating at an unknown value (not below examples/), select its own SDK path via the command "NEON-MDF: select Dan SDK path..." first. still creating app (build might fail)?`,
      "still create",
      "deactivate"
    );
    if (proceed !== "still create") {
      return;
    }
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
    "open in of each number"
  );
  if (openNow) {
    await vscode.commands.executeCommand("vscode.openFolder", destDir, { forceNewWindow: true });
  }
}
