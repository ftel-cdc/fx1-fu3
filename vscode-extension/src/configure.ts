// configure.ts - dam bao build dir cua 1 preset da duoc CMake configure VA
// da co CMake File API query tu TRUOC lan configure do (query phai co
// TRUOC thi CMake moi tra loi - xem cmakeFileApi.ts). Duoc goi o DAU moi
// command build/flash/debug - day la thay the cho cac task rieng "CMake:
// configure (...)" da bi xoa khoi tasks.json.
import * as vscode from "vscode";
import { ensureFileApiQuery, hasFileApiReply } from "./cmakeFileApi";
import { runCMakeConfigure } from "./cmakeRunner";

async function fileExists(uri: vscode.Uri): Promise<boolean> {
  try {
    await vscode.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}

export function buildDirFor(cwd: vscode.Uri, preset: string): vscode.Uri {
  return vscode.Uri.joinPath(cwd, "build", preset);
}

/**
 * Tra ve true neu build dir da san sang de build/debug (co CMakeCache.txt
 * VA co CMake File API reply) - tu dong chay `cmake --preset <preset>` neu
 * chua (lan dau mo project, hoac project da configure TRUOC KHI extension
 * ghi query file lan dau, nen chua co reply).
 */
export async function ensureConfigured(cwd: vscode.Uri, preset: string): Promise<boolean> {
  const buildDir = buildDirFor(cwd, preset);
  const alreadyConfigured = await fileExists(vscode.Uri.joinPath(buildDir, "CMakeCache.txt"));
  const alreadyHasReply = await hasFileApiReply(buildDir);

  await ensureFileApiQuery(buildDir);

  if (alreadyConfigured && alreadyHasReply) {
    return true;
  }

  const exitCode = await runCMakeConfigure(preset, cwd);
  return exitCode === 0;
}
