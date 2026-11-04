// cmakeFileApi.ts - dung CMake File API (khong parse output/cache) de lay
// danh sach executable THAT cua 1 build dir (ten + duong dan artifact) -
// day la co che CHINH THUC CMake cung cap cho tool ben ngoai doc project
// model (CMake Tools/CLion... deu dung co che nay), cho phep extension
// KHONG can hardcode ten executable (blinky/bootloader/...) rieng cho tung
// project vi du - tu dong ap dung cho project moi mien la co CMakePresets.json.
// Xem https://cmake.org/cmake/help/latest/manual/cmake-file-api.7.html
import * as vscode from "vscode";

export interface DiscoveredExecutable {
  name: string;
  /** Duong dan TUYET DOI toi file thuc thi (.elf/khong duoi). */
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
 * Ghi query file (idempotent - chi ghi neu chua co). PHAI ton tai TRUOC
 * luc CMake configure/generate chay thi CMake moi ghi reply tuong ung -
 * goi ham nay TRUOC moi lan configure (xem configure.ts), khong phai sau.
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

// Query file o day dung dang "grouped" (1 file query.json voi mang
// "requests") - dang nay tra loi qua reply["<client>"]["query.json"].responses[]
// (MOI response co "kind"+"jsonFile"), KHAC voi dang "stateless" (tung file
// query rieng ten theo kind, vd "codemodel-v2") tra loi truc tiep qua
// reply["<client>"]["codemodel-v2"] - da xac nhan bang test that tren build
// dir thuc te (Blinky/WEACT_F412), lam ho khi gia dinh nham dang stateless
// luc dau (reply["<client>"]["codemodel-v2"] la undefined trong thuc te).
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
 * Doc danh sach executable THAT trong build dir qua CMake File API - goi
 * SAU KHI da chac chan configure xong VA co reply (xem configure.ts). Neu
 * chua tung configure voi query file nay, reply chua ton tai - tra ve []
 * thay vi throw, noi goi tu quyet dinh thong bao gi.
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
