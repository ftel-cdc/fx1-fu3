// configure.ts - ensures that the build dir of a preset has been configured by CMake and that CMake
// File API query is already in place before that configuration (the query must be in place for
// CMake to check for errors - see cmakeFileApi.ts). called at the beginning of each command
// build/flash/debug. The specific CMake configure task has been disposed
// from the tasks.json block.
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
 * check for true if build dir has been scaled to build/debug (with CMakeCache.txt and CMake File API
 * reply) - automatically run `cmake --preset <preset>` if not yet (first time open project, or the
 * project was configured before extension write query file first time, so no reply).
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
