// - without hardcoding or parsing the fragile mcu_variants.cmake format. This is the single
// diem to fix if later add new target and neon-mdf/cmake/presets/targets.json - extension
// automatically sees, no need to fix source.
//
// intentionally did not resolve CMake's preset-inheritance tap features (`inherits`, `condition`) -
// only read field directly + include phang 1 level. if a preset file in the future uses but that
// feature, this function will ignore (not crash) seen because of mis guessing.
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

/** Merge configurePresets from the root file and its includes into one target list, with one include level and no recursion. */
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
 * read <workspaceFolder>/CMakePresets.json + include of no, look up the target list (empty if no file
 * is found or parsable - use this handle instead, e.g. currently "not project neon-mdf").
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
