// debug.ts - enable real GDB debug (cortex-debug via OpenOCD) for existing executables in the dir
// build of the selected target. do not hardcode specific executable name (blinky/bootloader/...)
// for each project - from read through CMake File API (../cmakeFileApi.ts) after configured
// ensured by ../configure.ts. This replaces per-app .vscode/launch.json configuration
// specific configuration debug for each old project.
//
// Infer the OpenOCD .cfg and SVD paths from repository conventions; see
// flash_targets.cmake):
//   openocd cfg: <SDK root>/cmake/openocd/<target>_openocd.cfg (file DUY
//                most, no child separate copy in each app from 2026-08-10 - see cmake/openocd/WEACT_F412_openocd.cfg)
//   svd file:    <SDK root>/mcu-description/generated/<target>.svd
// "SDK root" via resolveSdkPathUri() (../sdkPath.ts) - use setting neonMdf.sdkPath if the user has
// selected it separately, otherwise the dan convention varies from workspaceFolder (2 levels:
// <App>/ -> examples/ -> root -> neon-mdf/). not read from each field variable ${env:NEON_MDF_PATH}
// - verified that this environment variable is unset when VS Code starts from Finder/Dock without inheriting
// .zshrc of the shell).
import * as vscode from "vscode";
import { NeonMdfState } from "../state";
import { ensureConfigured, buildDirFor } from "../configure";
import { readExecutables, DiscoveredExecutable } from "../cmakeFileApi";
import { resolveSdkPathUri } from "../sdkPath";
import { runCMakeTarget } from "../cmakeRunner";
import { resolveDebugArchProfile } from "../debugArch";

async function fileExists(uri: vscode.Uri): Promise<boolean> {
  try {
    await vscode.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}

interface DebugTargetsManifest {
  bootloader?: string;
  factory?: string;
  ota: Set<string>;
}

// read "<builddir>/neon_mdf_debug_targets.txt" - write by
// cmake/flash_targets.cmake's neon_mdf_add_flash_targets()/
// neon_mdf_add_ota_flash_target(), specifies a "key=value" dynamic per level (bootloader=<name>,
// factory=<name>, ota=<name> - may have multiple "ota=" dynamics in the future if an app has
// multiple OTA executables use a Set). Return null if the manifest is absent, for apps without a
// bootloader:, see debugCommand()).
async function readDebugTargetsManifest(buildDir: vscode.Uri): Promise<DebugTargetsManifest | null> {
  const manifestUri = vscode.Uri.joinPath(buildDir, "neon_mdf_debug_targets.txt");
  if (!(await fileExists(manifestUri))) {
    return null;
  }
  const raw = await vscode.workspace.fs.readFile(manifestUri);
  const text = Buffer.from(raw).toString("utf-8");
  const manifest: DebugTargetsManifest = { ota: new Set<string>() };
  for (const line of text.split("\n")) {
    const trimmed = line.trim();
    if (!trimmed) {
      continue;
    }
    const idx = trimmed.indexOf("=");
    if (idx < 0) {
      continue;
    }
    const key = trimmed.slice(0, idx);
    const value = trimmed.slice(idx + 1);
    if (key === "bootloader") {
      manifest.bootloader = value;
    } else if (key === "factory") {
      manifest.factory = value;
    } else if (key === "ota") {
      manifest.ota.add(value);
    }
  }
  return manifest;
}

export async function debugCommand(state: NeonMdfState): Promise<void> {
  const folder = vscode.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode.window.showWarningMessage("NEON-MDF: no workspace folder opened.");
    return;
  }
  if (!state.target) {
    vscode.window.showWarningMessage(
      'NEON-MDF: No target selected. Use Select Target in the status bar first.'
    );
    return;
  }

  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode.window.showErrorMessage(`NEON-MDF: configure failed (target ${state.target}) - see terminal.`);
    return;
  }

  const buildDir = buildDirFor(folder.uri, state.target);
  const executables = await readExecutables(buildDir);
  if (executables.length === 0) {
    vscode.window.showWarningMessage(
      `NEON-MDF: No executable found for target ${state.target}. Configuration completed, but the CMake File API returned no EXECUTABLE target.`
    );
    return;
  }

  let chosen: DiscoveredExecutable;
  if (executables.length === 1) {
    chosen = executables[0];
  } else {
    const picked = await vscode.window.showQuickPick(
      executables.map((e) => ({ label: e.name, exe: e })),
      { placeHolder: "select executable to debug" }
    );
    if (!picked) {
      return;
    }
    chosen = picked.exe;
  }

  const openocdCfg = vscode.Uri.joinPath(
    resolveSdkPathUri(folder.uri),
    "cmake",
    "openocd",
    `${state.target}_openocd.cfg`
  );
  if (!(await fileExists(openocdCfg))) {
    vscode.window.showErrorMessage(`NEON-MDF: could not find openocd cfg file: ${openocdCfg.fsPath}`);
    return;
  }

  const svdFile = vscode.Uri.joinPath(
    resolveSdkPathUri(folder.uri),
    "mcu-description",
    "generated",
    `${state.target}.svd`
  );
  const hasSvd = await fileExists(svdFile);

  // Flash first via CMake target (flash_jtag/flash_ota) - not to cortex-debug from flash using GDB's
  // original GDB `load` command.
  // + verify on hardware): `load` write only using ELF (vector table+code, starting from
  // <partition>_VECTOR_BASE) - but header image_header_t (magic/size/CRC, 512 bytes RIGHT before
  // vector table) contains 1 flash sector with code, not part of ELF (that's
  // build_full_image.py/build_ota_image.py from ghep add when build blob to real flash, see
  // cmake/flash_targets.cmake) - write flash requires erase the entire sector first, `load` erase
  // lose header without rewriting (doesn't know no exists) -> header into 0xFFFFFFFF -> bootloader
  // reset and found mismatched header -> can't guarantee jumping app -> CPU connected in
  // bootloader. A runToEntryPoint breakpoint based on app.elf symbols does not
  // indicate matching time -> "running but not used in main" + step error "Cannot find bounds of
  // current function" (is debugging wrong bootloader code using symbol table of app.elf). true
  // confirmation: read again 0x08008000 after GDB `load` output all 0xFFFFFFFF (previously, from
  // flash_jtag, was mismatched header).
  //
  // select flash_ota/flash_jtag: read real manifest "<builddir>/neon_mdf_debug_targets.txt" (write by
  // cmake/flash_targets.cmake, intending "factory=<name>"/"ota=<name>"/ "bootloader=<name>") see
  // because guessed executable name has executable "_ota" not true - real bug gap:
  // examples/OTA_Demo has executable "ota_demo_ota_bad" (used for test rollback) share linker
  // script/slot with "ota_demo_ota" but not executable name "_ota" -> old heuristic mistakenly
  // selected flash_jtag (flash "ota_demo" factory) and still attach debugger with symbol/entry
  // entry point for ota_demo_ota_bad, causing incorrect breakpoints. An executable absent from the
  // manifest (same as ota_demo_ota_bad) no real automatic flash path (see comment in
  // examples/OTA_Demo/CMakeLists.txt - must be from CMake manual to test) - clear error seen
  // because mistaken flash and debug wrong process.
  const manifest = await readDebugTargetsManifest(buildDir);
  let flashTargetName: string;
  if (manifest) {
    if (manifest.ota.has(chosen.name)) {
      flashTargetName = "flash_ota";
    } else if (chosen.name === manifest.factory || chosen.name === manifest.bootloader) {
      flashTargetName = "flash_jtag";
    } else {
      vscode.window.showErrorMessage(
        `NEON-MDF: executable "${chosen.name}" not assigned flash target automatically ` +
          `(not bootloader/factory/OTA according to CMakeLists.txt of this app) - cannot ` +
          `debug via "NEON-MDF: Debug" because I don't know what to flash first. See CMakeLists.txt ` +
          `of app to flash/test curved message (e.g. the "_bad" version of examples/OTA_Demo used for testing ` +
          `rollback, need to fix CMake temporarily - see comment in that file).`
      );
      return;
    }
  } else {
    // no manifest (e.g. target has no bootloader: as FU2 - does not call neon_mdf_add_flash_targets() so
    // there is never flash_jtag/flash_ota) - keeps the same old heuristic to not regress:
    // flash_jtag produces a clear error from runCMakeTarget() below if that CMake
    // target does not exist.
    flashTargetName = chosen.name.endsWith("_ota") ? "flash_ota" : "flash_jtag";
  }
  const flashExitCode = await runCMakeTarget({
    taskName: `NEON-MDF: flash before Debug (${flashTargetName})`,
    preset: state.target,
    cmakeTarget: flashTargetName,
    cwd: folder.uri,
  });
  if (flashExitCode !== 0) {
    vscode.window.showErrorMessage(
      `NEON-MDF: flash before debug fails (target CMake "${flashTargetName}") - see terminal. Debug aborted.`
    );
    return;
  }

  // TAM time hardcode "cortex-m" - both FU2 times the current WEACT_F412 are Cortex-M, no real RISC-V
  // target used in mcu-description to read arch: true (see TODO only tiet in debugArch.ts's
  // resolveDebugArchProfile()). when adding the first RISC-V target: modify this action to read
  // arch: true (e.g. from 1 JSON manifest each created by generate.py, or extension from YAML
  // device description). Do not guess a RISC-V setup before its toolchain and debug protocol are verified; see
  // exchange
  // 2026-08-10 for CH32V003F4P6).
  const archProfile = resolveDebugArchProfile("cortex-m");

  const config: vscode.DebugConfiguration = {
    name: `NEON-MDF: Debug ${chosen.name} (${state.target})`,
    type: archProfile.debugType,
    request: "launch",
    cwd: folder.uri.fsPath,
    executable: chosen.artifactPath,
    servertype: "openocd",
    configFiles: [openocdCfg.fsPath],
    device: state.target,
    runToEntryPoint: "main",
    // [] (not false - read source cortex-debug: false still running normal `target-download`, only empty
    // network []/list per instance ignoring download) - already flashed using the above method
    // (flash_jtag/ flash_ota, keep header intact), do not let cortex-debug flash again old time
    // using GDB load, which would erase the image header; see above. This behavior applies to all
    // each live kien (still leaves header/flashmap of this SDK itself, not ARM letter dac) - not
    // male in debugArch.ts.
    loadFiles: [],
    // workaround command reset DAC mail BY KIEN live - see debugArch.ts.
    preLaunchCommands: archProfile.preLaunchCommands,
    postLaunchCommands: archProfile.postLaunchCommands,
  };
  if (hasSvd) {
    config.svdFile = svdFile.fsPath;
  }

  const started = await vscode.debug.startDebugging(folder, config);
  if (!started) {
    vscode.window.showErrorMessage(
      "NEON-MDF: Could not start the debug session. Check that cortex-debug (marus25.cortex-debug) is enabled."
    );
  }
}
