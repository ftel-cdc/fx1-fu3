// debug.ts - bat phien debug GDB THAT (cortex-debug qua OpenOCD) cho
// executable dang co trong build dir cua target dang chon. KHONG hardcode
// ten executable (blinky/bootloader/...) rieng cho tung project - tu doc
// qua CMake File API (../cmakeFileApi.ts) sau khi dam bao da configure
// (../configure.ts). Day la thay the cho .vscode/launch.json - khong con
// can khai bao cau hinh debug rieng cho tung project nua.
//
// openocd cfg + svd file suy ra THEO QUY UOC da dung xuyen suot repo (xem
// flash_targets.cmake):
//   openocd cfg: <SDK root>/cmake/openocd/<target>_openocd.cfg (file DUY
//                NHAT, khong con ban sao rieng trong tung app tu 2026-08-10 -
//                xem cmake/openocd/WEACT_F412_openocd.cfg)
//   svd file:    <SDK root>/mcu-description/generated/<target>.svd
// "SDK root" qua resolveSdkPathUri() (../sdkPath.ts) - dung setting
// neonMdf.sdkPath NEU nguoi dung da chon rieng, khong thi quy uoc duong dan
// TUONG DOI tu workspaceFolder (2 cap: <App>/ -> examples/ -> goc ->
// neon-mdf/). KHONG doc tu bien moi truong ${env:NEON_MDF_PATH} - da
// xac nhan bang thuc te bien nay RONG khi VS Code mo tu Finder/Dock (khong
// ke thua .zshrc cua shell).
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

// Doc "<builddir>/neon_mdf_debug_targets.txt" - ghi boi
// cmake/flash_targets.cmake's neon_mdf_add_flash_targets()/
// neon_mdf_add_ota_flash_target(), dinh dang 1 dong "key=value" moi cap
// (bootloader=<ten>, factory=<ten>, ota=<ten> - co the co nhieu dong "ota="
// trong tuong lai neu 1 app co nhieu slot OTA, gom het vao 1 Set). Tra ve
// null neu file chua ton tai (app khong co bootloader:, xem debugCommand()).
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
    vscode.window.showWarningMessage("NEON-MDF: chua mo workspace folder nao.");
    return;
  }
  if (!state.target) {
    vscode.window.showWarningMessage(
      'NEON-MDF: chua chon target - bam vao "chon target" tren status bar truoc.'
    );
    return;
  }

  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode.window.showErrorMessage(`NEON-MDF: configure that bai (target ${state.target}) - xem terminal.`);
    return;
  }

  const buildDir = buildDirFor(folder.uri, state.target);
  const executables = await readExecutables(buildDir);
  if (executables.length === 0) {
    vscode.window.showWarningMessage(
      `NEON-MDF: khong tim thay executable nao trong build dir cua target ${state.target} (da configure nhung CMake File API khong tra ve target EXECUTABLE nao).`
    );
    return;
  }

  let chosen: DiscoveredExecutable;
  if (executables.length === 1) {
    chosen = executables[0];
  } else {
    const picked = await vscode.window.showQuickPick(
      executables.map((e) => ({ label: e.name, exe: e })),
      { placeHolder: "Chon executable de debug" }
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
    vscode.window.showErrorMessage(`NEON-MDF: khong tim thay file openocd cfg: ${openocdCfg.fsPath}`);
    return;
  }

  const svdFile = vscode.Uri.joinPath(
    resolveSdkPathUri(folder.uri),
    "mcu-description",
    "generated",
    `${state.target}.svd`
  );
  const hasSvd = await fileExists(svdFile);

  // Flash TRUOC qua dung target CMake (flash_jtag/flash_ota) - KHONG de
  // cortex-debug tu flash bang lenh `load` goc cua GDB. Ly do (bug that gap
  // + verify tren phan cung): `load` chi ghi dung noi dung ELF (vector
  // table+code, bat dau tu <partition>_VECTOR_BASE) - nhung header
  // image_header_t (magic/size/CRC, 512 byte NGAY TRUOC vector table) nam
  // CHUNG 1 sector flash voi code, KHONG phai 1 phan cua ELF (do
  // build_full_image.py/build_ota_image.py tu ghep them khi build blob de
  // flash that, xem cmake/flash_targets.cmake) - ghi flash bat buoc xoa ca
  // sector truoc, `load` xoa mat header ma khong ghi lai (khong biet no ton
  // tai) -> header thanh 0xFFFFFFFF -> bootloader reset xong thay header
  // khong hop le -> KHONG dam nhay vao app -> CPU ket qua o bootloader,
  // breakpoint tam cua runToEntryPoint (dat theo symbol cua app.elf) khong
  // bao gio khop -> "chay nhung khong dung o main" + step bao loi "Cannot
  // find bounds of current function" (dang debug nham code bootloader bang
  // symbol table cua app.elf). Da xac nhan THAT: doc lai 0x08008000 sau khi
  // GDB `load` cho ra toan 0xFFFFFFFF (truoc do, tu flash_jtag, la header
  // hop le).
  //
  // Chon flash_ota/flash_jtag: doc manifest THAT
  // "<builddir>/neon_mdf_debug_targets.txt" (ghi boi
  // cmake/flash_targets.cmake, dinh dang "factory=<ten>"/"ota=<ten>"/
  // "bootloader=<ten>") thay vi DOAN qua ten executable co ket thuc "_ota"
  // hay khong - bug that da gap: examples/OTA_Demo co executable
  // "ota_demo_ota_bad" (dung de test rollback) dung CHUNG linker script/slot
  // voi "ota_demo_ota" nhung ten KHONG ket thuc "_ota" -> heuristic cu chon
  // nham flash_jtag (flash "ota_demo" factory) roi van attach debugger voi
  // symbol/entry point cua "ota_demo_ota_bad" -> breakpoint sai dia chi.
  // Executable khong khop ten nao trong manifest (dung nhu ota_demo_ota_bad)
  // KHONG co duong flash tu dong that su (xem comment trong
  // examples/OTA_Demo/CMakeLists.txt - phai tu sua CMake tay de test) -
  // bao loi ro rang thay vi flash nham roi debug sai chuong trinh.
  const manifest = await readDebugTargetsManifest(buildDir);
  let flashTargetName: string;
  if (manifest) {
    if (manifest.ota.has(chosen.name)) {
      flashTargetName = "flash_ota";
    } else if (chosen.name === manifest.factory || chosen.name === manifest.bootloader) {
      flashTargetName = "flash_jtag";
    } else {
      vscode.window.showErrorMessage(
        `NEON-MDF: executable "${chosen.name}" khong duoc gan flash target tu dong nao ` +
          `(khong phai bootloader/factory/OTA theo CMakeLists.txt cua app nay) - khong the ` +
          `debug qua "NEON-MDF: Debug" vi khong biet phai flash gi truoc. Xem CMakeLists.txt ` +
          `cua app de flash/test thu cong (vd ban "_bad" cua examples/OTA_Demo dung de test ` +
          `rollback, can sua CMake tam thoi - xem comment trong file do).`
      );
      return;
    }
  } else {
    // Khong co manifest (vd target khong co bootloader: nhu FU2 - khong goi
    // neon_mdf_add_flash_targets() nen chua tung co flash_jtag/flash_ota) -
    // giu nguyen heuristic cu de khong regress: flash_jtag se tu bao loi ro
    // rang qua runCMakeTarget() ben duoi neu target CMake do khong ton tai.
    flashTargetName = chosen.name.endsWith("_ota") ? "flash_ota" : "flash_jtag";
  }
  const flashExitCode = await runCMakeTarget({
    taskName: `NEON-MDF: flash truoc khi Debug (${flashTargetName})`,
    preset: state.target,
    cmakeTarget: flashTargetName,
    cwd: folder.uri,
  });
  if (flashExitCode !== 0) {
    vscode.window.showErrorMessage(
      `NEON-MDF: flash truoc khi Debug that bai (target CMake "${flashTargetName}") - xem terminal. Debug bi huy.`
    );
    return;
  }

  // TAM THOI hardcode "cortex-m" - CA FU2 lan WEACT_F412 hien tai deu la
  // Cortex-M, chua co target RISC-V nao that su ton tai trong
  // mcu-description de doc arch: that (xem TODO chi tiet trong
  // debugArch.ts's resolveDebugArchProfile()). Khi them target RISC-V dau
  // tien: sua dong nay thanh doc arch that (vd tu 1 manifest JSON moi do
  // generate.py sinh, hoac extension tu parse device YAML) - CHUA lam
  // truoc vi chua biet toolchain/debug protocol that se dung (xem trao doi
  // 2026-08-10 ve CH32V003F4P6).
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
    // [] (KHONG PHAI false - da doc source cortex-debug: false van chay
    // `target-download` binh thuong, CHI mang []/list rong moi thuc su bo
    // qua buoc download) - da flash dung cach o tren roi (flash_jtag/
    // flash_ota, giu nguyen header), KHONG de cortex-debug flash lai lan
    // nua bang `load` goc (se xoa mat header, xem giai thich o tren). Day
    // la hanh vi CHUNG cho moi kien truc (van de header/flashmap cua chinh
    // SDK nay, khong phai dac thu ARM) - khong nam trong debugArch.ts.
    loadFiles: [],
    // Lenh workaround reset DAC THU THEO KIEN TRUC - xem debugArch.ts.
    preLaunchCommands: archProfile.preLaunchCommands,
    postLaunchCommands: archProfile.postLaunchCommands,
  };
  if (hasSvd) {
    config.svdFile = svdFile.fsPath;
  }

  const started = await vscode.debug.startDebugging(folder, config);
  if (!started) {
    vscode.window.showErrorMessage(
      "NEON-MDF: khong khoi dong duoc phien debug - kiem tra extension \"cortex-debug\" (marus25.cortex-debug) da cai chua."
    );
  }
}
