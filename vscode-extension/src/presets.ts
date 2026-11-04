// presets.ts - doc CMakePresets.json (+ include cua no) de lay danh sach
// target THAT (vd FU2, WEACT_F412) - KHONG hardcode, KHONG parse
// mcu_variants.cmake (qua fragile, khong phai machine-readable). Day la
// diem duy nhat can sua neu sau nay them target moi vao
// neon-mdf/cmake/presets/targets.json - extension tu dong thay, khong can
// sua source.
//
// CO Y THUC KHONG resolve cac tinh nang preset-inheritance phuc tap cua
// CMake (`inherits`, `condition`) - chi doc field truc tiep + include phang
// 1 cap. Neu 1 file preset trong tuong lai dung nhung tinh nang do, ham nay
// se BO QUA (khong crash) thay vi doan sai.
import * as vscode from "vscode";

export interface TargetInfo {
  name: string;
  displayName: string;
}

interface CMakeConfigurePreset {
  name: string;
  displayName?: string;
  hidden?: boolean;
}

interface CMakePresetsFile {
  include?: string[];
  configurePresets?: CMakeConfigurePreset[];
}

async function readJsonFile(uri: vscode.Uri): Promise<CMakePresetsFile | undefined> {
  try {
    const bytes = await vscode.workspace.fs.readFile(uri);
    const text = Buffer.from(bytes).toString("utf8");
    return JSON.parse(text) as CMakePresetsFile;
  } catch {
    return undefined;
  }
}

/** Gop configurePresets tu 1 file goc + cac file include (1 cap, khong de quy) thanh danh sach target duy nhat. */
function collectTargets(files: CMakePresetsFile[]): TargetInfo[] {
  const seen = new Set<string>();
  const targets: TargetInfo[] = [];
  for (const file of files) {
    for (const preset of file.configurePresets ?? []) {
      if (preset.hidden) {
        continue;
      }
      if (seen.has(preset.name)) {
        continue;
      }
      seen.add(preset.name);
      targets.push({ name: preset.name, displayName: preset.displayName ?? preset.name });
    }
  }
  return targets;
}

/**
 * Doc <workspaceFolder>/CMakePresets.json + include cua no, tra ve danh
 * sach target (rong neu khong tim thay file hoac khong parse duoc - goi
 * noi dung tu xu ly truong hop nay, vd hien "chua phai project neon-mdf").
 */
export async function discoverTargets(workspaceFolder: vscode.Uri): Promise<TargetInfo[]> {
  const rootUri = vscode.Uri.joinPath(workspaceFolder, "CMakePresets.json");
  const root = await readJsonFile(rootUri);
  if (!root) {
    return [];
  }

  const files: CMakePresetsFile[] = [root];
  for (const rel of root.include ?? []) {
    const includeUri = vscode.Uri.joinPath(workspaceFolder, rel);
    const fragment = await readJsonFile(includeUri);
    if (fragment) {
      files.push(fragment);
    }
  }

  return collectTargets(files);
}
