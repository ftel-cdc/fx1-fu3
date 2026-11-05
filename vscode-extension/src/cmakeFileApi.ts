// Read executable targets and artifact paths from the CMake File API.
// Use the grouped client query and its actual codemodel-v2 reply rather than
// guessing build-directory names or executable filenames.
import * as vscode from "vscode";

export interface DiscoveredExecutable {
  name: string;
  /** extension, then resolve the actual file path (.elf or no suffix). */
  artifactPath: string;
}

const CLIENT_NAME = "client-neonmdf";

function queryFile(buildDir: vscode.Uri): vscode.Uri {
  return vscode.Uri.joinPath(buildDir, ".cmake", "api", "v1", "query", CLIENT_NAME, "query.json");
}

function replyDir(buildDir: vscode.Uri): vscode.Uri {
  return vscode.Uri.joinPath(buildDir, ".cmake", "api", "v1", "reply");
}

async function fileExists(uri: vscode.Uri): Promise<boolean> {
  try {
    await vscode.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}

/**
 * write query file (idempotent - only write if not already). must exist before CMake
 * configure/generate runs, so CMake writes a corresponding reply every time - call this function
 * before each configuration (see configure.ts), not after.
 */
export async function ensureFileApiQuery(buildDir: vscode.Uri): Promise<void> {
  const file = queryFile(buildDir);
  if (await fileExists(file)) {
    return;
  }
  await vscode.workspace.fs.createDirectory(vscode.Uri.joinPath(file, ".."));
  const content = Buffer.from(JSON.stringify({ requests: [{ kind: "codemodel", version: 2 }] }, null, 2), "utf8");
  await vscode.workspace.fs.writeFile(file, content);
}

export async function hasFileApiReply(buildDir: vscode.Uri): Promise<boolean> {
  return fileExists(replyDir(buildDir));
}

async function readJson<T>(uri: vscode.Uri): Promise<T> {
  const bytes = await vscode.workspace.fs.readFile(uri);
  return JSON.parse(Buffer.from(bytes).toString("utf8")) as T;
}

// The client query is stateful: one query.json contains a requests array. This was
// checking for errors via reply["<client>"]["query.json"].responses[] (each response has
// "kind"+"jsonFile"), unlike being "stateless" (each query file is named after its own kind, e.g.
// "codemodel-v2") which checks for errors directly via reply["<client>"]["codemodel-v2"] -
// verified against Blinky/WEACT_F412 build output. A stateless reply assumption fails because
// reply["<client>"]["codemodel-v2"] is undefined.
interface ReplyIndex {
  reply: {
    [client: string]: {
      "query.json"?: {
        responses: { kind: string; jsonFile: string }[];
      };
    };
  };
}

interface CodemodelTargetRef {
  jsonFile: string;
}

interface Codemodel {
  configurations: { targets: CodemodelTargetRef[] }[];
}

interface CodemodelTarget {
  name: string;
  type: string;
  artifacts?: { path: string }[];
}

/**
 * read the actual executable list in build dir via CMake File API - call after configure has been done
 * and there is a reply (see configure.ts). if never configured with this query file, the reply does
 * not exist - look up [] because throw, we call from what message specifier.
 */
export async function readExecutables(buildDir: vscode.Uri): Promise<DiscoveredExecutable[]> {
  const reply = replyDir(buildDir);
  let entries: [string, vscode.FileType][];
  try {
    entries = await vscode.workspace.fs.readDirectory(reply);
  } catch {
    return [];
  }

  const indexFile = entries
    .map(([name]) => name)
    .filter((name) => name.startsWith("index-") && name.endsWith(".json"))
    .sort()
    .pop();
  if (!indexFile) {
    return [];
  }

  const index = await readJson<ReplyIndex>(vscode.Uri.joinPath(reply, indexFile));
  const responses = index.reply[CLIENT_NAME]?.["query.json"]?.responses ?? [];
  const codemodelResponse = responses.find((r) => r.kind === "codemodel");
  if (!codemodelResponse) {
    return [];
  }

  const codemodel = await readJson<Codemodel>(vscode.Uri.joinPath(reply, codemodelResponse.jsonFile));
  const targetRefs = codemodel.configurations[0]?.targets ?? [];

  const executables: DiscoveredExecutable[] = [];
  for (const ref of targetRefs) {
    const target = await readJson<CodemodelTarget>(vscode.Uri.joinPath(reply, ref.jsonFile));
    if (target.type !== "EXECUTABLE" || !target.artifacts || target.artifacts.length === 0) {
      continue;
    }
    executables.push({
      name: target.name,
      artifactPath: vscode.Uri.joinPath(buildDir, target.artifacts[0].path).fsPath,
    });
  }
  return executables;
}
