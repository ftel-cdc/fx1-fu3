// sdkPath.ts - "neonMdf.sdkPath" giong idf.espIdfPath cua ESP-IDF: duong
// dan TUYET DOI toi neon-mdf SDK, ghi de quy uoc suy doan-tu-vi-tri-tuong-doi
// (`${appDir}/../../neon-mdf`) ma toan bo repo (CMakeLists.txt,
// CMakePresets.json, .vscode/settings.json cua tung app) dang dung mac
// dinh. De trong (mac dinh) = van dung quy uoc cu nguyen ven - KHONG doi
// hanh vi hien co cua Blinky/FreeRTOS_Blinky.
//
// Quy uoc "2 cap" (<App>/ -> examples/ -> repo root -> neon-mdf/) - doi tu
// "3 cap" (qua Projects/Examples/) sau khi Projects/Examples/ chuyen thanh
// examples/ ngay duoi repo root, xoa Projects/ (2026-08-10).
//
// Setting nay la GLOBAL (khong phai Workspace nhu neonMdf.target/transport) -
// giong idf.espIdfPath, 1 may thuong chi dung 1 SDK checkout cho moi
// workspace, khong phai lua chon rieng tung app.
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
 * Bien moi truong bo sung cho cmake subprocess NEU nguoi dung da chon SDK
 * path rieng - undefined neu dang dung mac dinh (khong can can thiep, de
 * CMakeLists.txt/settings.json cua tung app tu quyet dinh nhu truoc gio).
 * CMakeLists.txt cua moi app deu co guard `if(NOT DEFINED ENV{NEON_MDF_PATH})`
 * nen bien nay, khi co, se GHI DE quy uoc tuong doi cu ma khong can sua file
 * nao ca - ap dung duoc cho ca app cu (Blinky...) lan app moi.
 */
export function sdkPathEnv(): Record<string, string> | undefined {
  const configured = getConfiguredSdkPath();
  return configured ? { NEON_MDF_PATH: configured } : undefined;
}

/**
 * SDK path THAT se ap dung cho 1 thu muc app (anchorDir): duong dan da
 * chon trong setting neu co, khong thi quy uoc cu (2 cap tu anchorDir len
 * neon-mdf/, dung `${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf` giong
 * CMakeLists.txt tinh - xem examples/<App>/CMakeLists.txt).
 */
export function resolveSdkPathUri(anchorDir: vscode.Uri): vscode.Uri {
  const configured = getConfiguredSdkPath();
  if (configured) {
    return vscode.Uri.file(configured);
  }
  return vscode.Uri.joinPath(anchorDir, "..", "..", "neon-mdf");
}

/**
 * Duong dan tuong doi kieu POSIX (dung thang duoc trong CMakeLists.txt/
 * CMakePresets.json, khong phu thuoc OS) tu 1 thu muc app toi SDK - dung
 * boi createApp.ts khi sinh project moi (thay the gia dinh co dinh
 * "../../neon-mdf" bang gia tri TINH THAT dua tren vi tri app + SDK
 * dang chon).
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

/** 1 thu muc CO PHAI la checkout neon-mdf SDK THAT hay khong - tim file marker tools/cmake/project.cmake. */
export async function looksLikeSdkRoot(dir: vscode.Uri): Promise<boolean> {
  return fileExists(vscode.Uri.joinPath(dir, "tools", "cmake", "project.cmake"));
}
