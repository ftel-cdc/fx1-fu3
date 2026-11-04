"use strict";
var __create = Object.create;
var __defProp = Object.defineProperty;
var __getOwnPropDesc = Object.getOwnPropertyDescriptor;
var __getOwnPropNames = Object.getOwnPropertyNames;
var __getProtoOf = Object.getPrototypeOf;
var __hasOwnProp = Object.prototype.hasOwnProperty;
var __export = (target, all) => {
  for (var name in all)
    __defProp(target, name, { get: all[name], enumerable: true });
};
var __copyProps = (to, from, except, desc) => {
  if (from && typeof from === "object" || typeof from === "function") {
    for (let key of __getOwnPropNames(from))
      if (!__hasOwnProp.call(to, key) && key !== except)
        __defProp(to, key, { get: () => from[key], enumerable: !(desc = __getOwnPropDesc(from, key)) || desc.enumerable });
  }
  return to;
};
var __toESM = (mod, isNodeMode, target) => (target = mod != null ? __create(__getProtoOf(mod)) : {}, __copyProps(
  // If the importer is in node compatibility mode or this is not an ESM
  // file that has been converted to a CommonJS file using a Babel-
  // compatible transform (i.e. "__esModule" has not been set), then set
  // "default" to the CommonJS "module.exports" for node compatibility.
  isNodeMode || !mod || !mod.__esModule ? __defProp(target, "default", { value: mod, enumerable: true }) : target,
  mod
));
var __toCommonJS = (mod) => __copyProps(__defProp({}, "__esModule", { value: true }), mod);

// src/extension.ts
var extension_exports = {};
__export(extension_exports, {
  activate: () => activate,
  deactivate: () => deactivate
});
module.exports = __toCommonJS(extension_exports);
var vscode20 = __toESM(require("vscode"));

// src/state.ts
var vscode = __toESM(require("vscode"));
var PORT_STATE_KEY = "neonMdf.selectedPort";
var NeonMdfState = class {
  constructor(context) {
    this.context = context;
  }
  _onDidChange = new vscode.EventEmitter();
  onDidChange = this._onDidChange.event;
  get config() {
    return vscode.workspace.getConfiguration("neonMdf");
  }
  get target() {
    return this.config.get("target", "");
  }
  async setTarget(target) {
    await this.config.update("target", target, vscode.ConfigurationTarget.Workspace);
    this._onDidChange.fire();
  }
  get transport() {
    return this.config.get("transport", "uart");
  }
  async setTransport(transport) {
    await this.config.update("transport", transport, vscode.ConfigurationTarget.Workspace);
    this._onDidChange.fire();
  }
  get baudRate() {
    const values = this.config.inspect("baudRate");
    return values?.workspaceFolderValue ?? values?.workspaceValue ?? values?.globalValue ?? (this.target === "FU2" ? 19200 : 115200);
  }
  get flashBaudRate() {
    return this.config.get("fu2FlashBaudRate", 19200);
  }
  get port() {
    return this.context.workspaceState.get(PORT_STATE_KEY);
  }
  async setPort(port) {
    await this.context.workspaceState.update(PORT_STATE_KEY, port);
    this._onDidChange.fire();
  }
  dispose() {
    this._onDidChange.dispose();
  }
};

// src/statusBar.ts
var vscode2 = __toESM(require("vscode"));
var NeonMdfStatusBar = class {
  constructor(state) {
    this.state = state;
    this.targetItem = vscode2.window.createStatusBarItem(vscode2.StatusBarAlignment.Left, 100);
    this.targetItem.command = "neonMdf.selectTarget";
    this.transportItem = vscode2.window.createStatusBarItem(vscode2.StatusBarAlignment.Left, 99);
    this.transportItem.command = "neonMdf.selectTransport";
    this.portItem = vscode2.window.createStatusBarItem(vscode2.StatusBarAlignment.Left, 98);
    this.portItem.command = "neonMdf.selectPort";
    this.subscription = state.onDidChange(() => this.refresh());
    this.refresh();
    this.targetItem.show();
    this.transportItem.show();
    this.portItem.show();
  }
  targetItem;
  transportItem;
  portItem;
  subscription;
  refresh() {
    this.targetItem.text = `$(circuit-board) ${this.state.target || "chon target"}`;
    this.targetItem.tooltip = "NEON-MDF: Chon Target";
    this.transportItem.text = this.state.transport === "jtag" ? "$(circuit-board) JTAG" : "$(plug) UART";
    this.transportItem.tooltip = "NEON-MDF: Chien luoc Flash (JTAG = qua SWD 1 phien, nhanh; UART = qua System Bootloader, can thao tac tay)";
    this.portItem.text = `$(radio-tower) ${this.state.port ?? "chua chon cong"}`;
    this.portItem.tooltip = "NEON-MDF: Chon Cong Serial";
  }
  dispose() {
    this.subscription.dispose();
    this.targetItem.dispose();
    this.transportItem.dispose();
    this.portItem.dispose();
  }
};

// src/commands/selectTarget.ts
var vscode4 = __toESM(require("vscode"));

// src/presets.ts
var vscode3 = __toESM(require("vscode"));
async function readJsonFile(uri) {
  try {
    const bytes = await vscode3.workspace.fs.readFile(uri);
    const text = Buffer.from(bytes).toString("utf8");
    return JSON.parse(text);
  } catch {
    return void 0;
  }
}
function collectTargets(files) {
  const seen = /* @__PURE__ */ new Set();
  const targets = [];
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
async function discoverTargets(workspaceFolder) {
  const rootUri = vscode3.Uri.joinPath(workspaceFolder, "CMakePresets.json");
  const root = await readJsonFile(rootUri);
  if (!root) {
    return [];
  }
  const files = [root];
  for (const rel of root.include ?? []) {
    const includeUri = vscode3.Uri.joinPath(workspaceFolder, rel);
    const fragment = await readJsonFile(includeUri);
    if (fragment) {
      files.push(fragment);
    }
  }
  return collectTargets(files);
}

// src/commands/selectTarget.ts
async function selectTargetCommand(state) {
  const folder = vscode4.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode4.window.showWarningMessage("NEON-MDF: chua mo workspace folder nao.");
    return;
  }
  const targets = await discoverTargets(folder.uri);
  if (targets.length === 0) {
    vscode4.window.showWarningMessage(
      "NEON-MDF: khong tim thay target nao trong CMakePresets.json cua workspace nay - dam bao dang mo dung 1 thu muc vi du (vd examples/Blinky), khong phai thu muc goc repo."
    );
    return;
  }
  const picked = await vscode4.window.showQuickPick(
    targets.map((t) => ({ label: t.displayName, description: t.name })),
    { placeHolder: "Chon target (doc tu CMakePresets.json)" }
  );
  if (!picked) {
    return;
  }
  await state.setTarget(picked.description ?? picked.label);
}

// src/commands/selectTransport.ts
var vscode5 = __toESM(require("vscode"));
async function selectTransportCommand(state) {
  const items = [
    {
      label: "$(plug) UART",
      description: state.target === "FU2" ? "ROMCODE USART update: chon boot mode 100, bam RESET khi tool san sang" : "System Memory Bootloader STM32: target flash_uart_stm32, thao tac B0/PB2 theo board",
      value: "uart"
    },
    {
      label: "$(circuit-board) JTAG/SWD",
      description: state.target === "FU2" ? "Chua ho tro: cau hinh OpenOCD FU2 dang la placeholder" : "Nap qua OpenOCD (target flash_jtag)",
      value: "jtag"
    }
  ];
  const picked = await vscode5.window.showQuickPick(items, { placeHolder: "Chon transport" });
  if (!picked) {
    return;
  }
  await state.setTransport(picked.value);
}

// src/commands/selectPort.ts
var vscode7 = __toESM(require("vscode"));

// src/ports.ts
var import_serialport = require("serialport");
async function listPorts() {
  const ports = await import_serialport.SerialPort.list();
  return ports.map((p) => ({
    path: p.path,
    manufacturer: p.manufacturer,
    vendorId: p.vendorId,
    productId: p.productId
  }));
}

// src/commands/detectPort.ts
var vscode6 = __toESM(require("vscode"));

// src/serialMonitor/portProbe.ts
var import_serialport2 = require("serialport");
var SYNC_BYTE = 85;
var READY_BYTE = 82;
var SYNC_RETRY_MS = 9500;
var SYNC_POLL_MS = 100;
var PROBE_BAUD_RATE = 115200;
function probeOnePort(path2) {
  return new Promise((resolve) => {
    let settled = false;
    let interval;
    const sp = new import_serialport2.SerialPort({ path: path2, baudRate: PROBE_BAUD_RATE, autoOpen: false });
    const finish = (found) => {
      if (settled) {
        return;
      }
      settled = true;
      if (interval) {
        clearInterval(interval);
      }
      clearTimeout(timeout);
      sp.removeAllListeners();
      if (sp.isOpen) {
        sp.close(() => {
        });
      }
      resolve(found);
    };
    const timeout = setTimeout(() => finish(false), SYNC_RETRY_MS);
    sp.on("error", () => finish(false));
    sp.open((err) => {
      if (err) {
        finish(false);
        return;
      }
      sp.flush(() => {
        if (settled) {
          return;
        }
        sp.on("data", (chunk) => {
          finish(chunk.length > 0 && chunk[0] === READY_BYTE);
        });
        interval = setInterval(() => sp.write(Buffer.from([SYNC_BYTE])), SYNC_POLL_MS);
      });
    });
  });
}
async function detectBootloaderPort() {
  const allCandidates = await listPorts();
  const candidates = allCandidates.filter((c) => c.vendorId !== void 0);
  if (candidates.length === 0) {
    return void 0;
  }
  const results = await Promise.all(candidates.map((c) => probeOnePort(c.path)));
  const foundIndex = results.findIndex((ok) => ok);
  return foundIndex >= 0 ? candidates[foundIndex].path : void 0;
}

// src/commands/detectPort.ts
async function detectPortCommand(state) {
  await vscode6.window.withProgress(
    {
      location: vscode6.ProgressLocation.Notification,
      title: "NEON-MDF: dang do cong serial - RESET/cap lai nguon board NGAY BAY GIO..."
    },
    async () => {
      const found = await detectBootloaderPort();
      if (!found) {
        vscode6.window.showWarningMessage(
          "NEON-MDF: khong tim thay cong nao dang chay bootloader trong cua so cho SYNC (~9.5s) - dam bao board vua duoc RESET/cap lai nguon dung luc lenh nay chay, hoac chon cong bang tay."
        );
        return;
      }
      await state.setPort(found);
      vscode6.window.showInformationMessage(`NEON-MDF: da tim thay bootloader tren cong ${found}.`);
    }
  );
}

// src/commands/selectPort.ts
var MANUAL_ENTRY = "$(edit) Nhap tay...";
var AUTO_DETECT_ENTRY = "$(zap) Tu dong do (RESET board truoc)...";
async function selectPortCommand(state) {
  let ports = [];
  try {
    ports = await listPorts();
  } catch (err) {
    vscode7.window.showWarningMessage(
      `NEON-MDF: khong doc duoc danh sach cong serial (${String(err)}) - nhap tay.`
    );
  }
  const items = [
    { label: AUTO_DETECT_ENTRY, description: "do cong dang chay bootloader (giao thuc SYNC/READY)" },
    ...ports.map((p) => ({
      label: p.path,
      description: p.manufacturer
    })),
    { label: MANUAL_ENTRY, description: "go duong dan cong serial" }
  ];
  const picked = await vscode7.window.showQuickPick(items, {
    placeHolder: ports.length > 0 ? "Chon cong serial" : "Khong tim thay cong nao - nhap tay hoac tu dong do"
  });
  if (!picked) {
    return;
  }
  if (picked.label === AUTO_DETECT_ENTRY) {
    await detectPortCommand(state);
    return;
  }
  let port = picked.label;
  if (picked.label === MANUAL_ENTRY) {
    const typed = await vscode7.window.showInputBox({
      prompt: "Duong dan cong serial (vd /dev/cu.usbmodem102, COM3, /dev/ttyACM0)",
      value: state.port ?? ""
    });
    if (!typed) {
      return;
    }
    port = typed;
  }
  await state.setPort(port);
}

// src/commands/build.ts
var vscode12 = __toESM(require("vscode"));

// src/cmakeRunner.ts
var vscode9 = __toESM(require("vscode"));

// src/sdkPath.ts
var vscode8 = __toESM(require("vscode"));
var path = __toESM(require("path"));
var SETTING_KEY = "sdkPath";
function getConfiguredSdkPath() {
  const raw = vscode8.workspace.getConfiguration("neonMdf").get(SETTING_KEY, "");
  const trimmed = raw?.trim();
  return trimmed ? trimmed : void 0;
}
async function setConfiguredSdkPath(sdkPath) {
  await vscode8.workspace.getConfiguration("neonMdf").update(SETTING_KEY, sdkPath ?? "", vscode8.ConfigurationTarget.Global);
}
function sdkPathEnv() {
  const configured = getConfiguredSdkPath();
  return configured ? { NEON_MDF_PATH: configured } : void 0;
}
function resolveSdkPathUri(anchorDir) {
  const configured = getConfiguredSdkPath();
  if (configured) {
    return vscode8.Uri.file(configured);
  }
  return vscode8.Uri.joinPath(anchorDir, "..", "..", "neon-mdf");
}
function relativePathToSdk(fromDir, sdkDir) {
  const rel = path.relative(fromDir.fsPath, sdkDir.fsPath);
  return rel.split(path.sep).join("/");
}
async function fileExists(uri) {
  try {
    await vscode8.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}
async function looksLikeSdkRoot(dir) {
  return fileExists(vscode8.Uri.joinPath(dir, "tools", "cmake", "project.cmake"));
}

// src/cmakeRunner.ts
var TASK_SOURCE = "NEON-MDF";
function runCMakeTarget(options) {
  const args = ["--build", "--preset", options.preset];
  if (options.cmakeTarget) {
    args.push("--target", options.cmakeTarget);
  }
  const shellExec = new vscode9.ShellExecution("cmake", args, {
    cwd: options.cwd.fsPath,
    // sdkPathEnv() chi tra ve gia tri neu nguoi dung da chon SDK path rieng
    // (neonMdf.sdkPath) - de trong thi khong can, de CMakeLists.txt/
    // settings.json cua tung app tu quyet dinh nhu truoc gio.
    env: { ...sdkPathEnv(), ...options.env }
  });
  const task = new vscode9.Task(
    { type: TASK_SOURCE, target: options.cmakeTarget ?? "all" },
    vscode9.TaskScope.Workspace,
    options.taskName,
    TASK_SOURCE,
    shellExec,
    []
  );
  task.presentationOptions = {
    reveal: vscode9.TaskRevealKind.Always,
    panel: vscode9.TaskPanelKind.Dedicated
  };
  if (options.background) {
    task.isBackground = true;
  }
  return new Promise((resolve) => {
    const disposable = vscode9.tasks.onDidEndTaskProcess((e) => {
      if (e.execution.task === task) {
        disposable.dispose();
        resolve(e.exitCode);
      }
    });
    vscode9.tasks.executeTask(task);
  });
}
function runCMakeConfigure(preset, cwd) {
  const shellExec = new vscode9.ShellExecution("cmake", ["--preset", preset], {
    cwd: cwd.fsPath,
    env: sdkPathEnv()
  });
  const task = new vscode9.Task(
    { type: TASK_SOURCE, target: "configure" },
    vscode9.TaskScope.Workspace,
    `NEON-MDF: configure (${preset})`,
    TASK_SOURCE,
    shellExec,
    []
  );
  task.presentationOptions = {
    reveal: vscode9.TaskRevealKind.Always,
    panel: vscode9.TaskPanelKind.Dedicated
  };
  return new Promise((resolve) => {
    const disposable = vscode9.tasks.onDidEndTaskProcess((e) => {
      if (e.execution.task === task) {
        disposable.dispose();
        resolve(e.exitCode);
      }
    });
    vscode9.tasks.executeTask(task);
  });
}

// src/configure.ts
var vscode11 = __toESM(require("vscode"));

// src/cmakeFileApi.ts
var vscode10 = __toESM(require("vscode"));
var CLIENT_NAME = "client-neonmdf";
function queryFile(buildDir) {
  return vscode10.Uri.joinPath(buildDir, ".cmake", "api", "v1", "query", CLIENT_NAME, "query.json");
}
function replyDir(buildDir) {
  return vscode10.Uri.joinPath(buildDir, ".cmake", "api", "v1", "reply");
}
async function fileExists2(uri) {
  try {
    await vscode10.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}
async function ensureFileApiQuery(buildDir) {
  const file = queryFile(buildDir);
  if (await fileExists2(file)) {
    return;
  }
  await vscode10.workspace.fs.createDirectory(vscode10.Uri.joinPath(file, ".."));
  const content = Buffer.from(JSON.stringify({ requests: [{ kind: "codemodel", version: 2 }] }, null, 2), "utf8");
  await vscode10.workspace.fs.writeFile(file, content);
}
async function hasFileApiReply(buildDir) {
  return fileExists2(replyDir(buildDir));
}
async function readJson(uri) {
  const bytes = await vscode10.workspace.fs.readFile(uri);
  return JSON.parse(Buffer.from(bytes).toString("utf8"));
}
async function readExecutables(buildDir) {
  const reply = replyDir(buildDir);
  let entries;
  try {
    entries = await vscode10.workspace.fs.readDirectory(reply);
  } catch {
    return [];
  }
  const indexFile = entries.map(([name]) => name).filter((name) => name.startsWith("index-") && name.endsWith(".json")).sort().pop();
  if (!indexFile) {
    return [];
  }
  const index = await readJson(vscode10.Uri.joinPath(reply, indexFile));
  const responses = index.reply[CLIENT_NAME]?.["query.json"]?.responses ?? [];
  const codemodelResponse = responses.find((r) => r.kind === "codemodel");
  if (!codemodelResponse) {
    return [];
  }
  const codemodel = await readJson(vscode10.Uri.joinPath(reply, codemodelResponse.jsonFile));
  const targetRefs = codemodel.configurations[0]?.targets ?? [];
  const executables = [];
  for (const ref of targetRefs) {
    const target = await readJson(vscode10.Uri.joinPath(reply, ref.jsonFile));
    if (target.type !== "EXECUTABLE" || !target.artifacts || target.artifacts.length === 0) {
      continue;
    }
    executables.push({
      name: target.name,
      artifactPath: vscode10.Uri.joinPath(buildDir, target.artifacts[0].path).fsPath
    });
  }
  return executables;
}

// src/configure.ts
async function fileExists3(uri) {
  try {
    await vscode11.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}
function buildDirFor(cwd, preset) {
  return vscode11.Uri.joinPath(cwd, "build", preset);
}
async function ensureConfigured(cwd, preset) {
  const buildDir = buildDirFor(cwd, preset);
  const alreadyConfigured = await fileExists3(vscode11.Uri.joinPath(buildDir, "CMakeCache.txt"));
  const alreadyHasReply = await hasFileApiReply(buildDir);
  await ensureFileApiQuery(buildDir);
  if (alreadyConfigured && alreadyHasReply) {
    return true;
  }
  const exitCode = await runCMakeConfigure(preset, cwd);
  return exitCode === 0;
}

// src/commands/build.ts
async function buildCommand(state) {
  const folder = vscode12.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode12.window.showWarningMessage("NEON-MDF: chua mo workspace folder nao.");
    return;
  }
  if (!state.target) {
    vscode12.window.showWarningMessage(
      'NEON-MDF: chua chon target - bam vao "chon target" tren status bar truoc.'
    );
    return;
  }
  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode12.window.showErrorMessage(`NEON-MDF: configure that bai (target ${state.target}) - xem terminal.`);
    return;
  }
  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: build (${state.target})`,
    preset: state.target,
    cwd: folder.uri
  });
  if (exitCode !== 0) {
    vscode12.window.showErrorMessage(`NEON-MDF: build that bai (target ${state.target}).`);
  }
}

// src/commands/flash.ts
var vscode13 = __toESM(require("vscode"));
async function fileExists4(uri) {
  try {
    await vscode13.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}
async function checkOpenocdCfg(state, folder) {
  const openocdCfg = vscode13.Uri.joinPath(resolveSdkPathUri(folder), "cmake", "openocd", `${state.target}_openocd.cfg`);
  if (!await fileExists4(openocdCfg)) {
    vscode13.window.showErrorMessage(`NEON-MDF: khong tim thay file openocd cfg: ${openocdCfg.fsPath}`);
    return false;
  }
  return true;
}
async function flashCommand(state, monitor) {
  const folder = vscode13.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode13.window.showWarningMessage("NEON-MDF: chua mo workspace folder nao.");
    return;
  }
  if (!state.target) {
    vscode13.window.showWarningMessage(
      'NEON-MDF: chua chon target - bam vao "chon target" tren status bar truoc.'
    );
    return;
  }
  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode13.window.showErrorMessage(`NEON-MDF: configure that bai (target ${state.target}) - xem terminal.`);
    return;
  }
  if (state.transport === "jtag") {
    await runFlashCombined(state, folder.uri);
    return;
  }
  await monitor.close();
  await runFlashUart(state, folder.uri);
}
async function runFlashCombined(state, folder) {
  if (state.target === "FU2") {
    vscode13.window.showErrorMessage("NEON-MDF: FU2 OpenOCD chua co cau hinh nap. Hay chon Transport UART de nap qua ROMCODE.");
    return void 0;
  }
  if (!await checkOpenocdCfg(state, folder)) {
    return void 0;
  }
  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: flash (${state.target}, SWD)`,
    preset: state.target,
    cmakeTarget: "flash_jtag",
    cwd: folder
  });
  if (exitCode !== 0) {
    vscode13.window.showErrorMessage(`NEON-MDF: flash qua SWD that bai (target ${state.target}) - xem terminal.`);
  }
  return exitCode;
}
async function runFlashUart(state, folder) {
  if (state.target !== "FU2") {
    return runFlashStm32Bootloader(state, folder);
  }
  if (!state.port) {
    vscode13.window.showWarningMessage("NEON-MDF: Chon cong serial truoc khi nap UART FU2.");
    return void 0;
  }
  if (!Number.isInteger(state.flashBaudRate) || state.flashBaudRate <= 0) {
    vscode13.window.showErrorMessage("NEON-MDF: neonMdf.fu2FlashBaudRate phai la so nguyen duong khop ROMCODE.");
    return void 0;
  }
  const exitCode = await runCMakeTarget({
    taskName: "NEON-MDF: FU2 ROMCODE UART update",
    preset: state.target,
    cmakeTarget: "flash_uart_fu2",
    cwd: folder,
    env: {
      NEON_MDF_SERIAL_PORT: state.port,
      NEON_MDF_SERIAL_BAUD: String(state.flashBaudRate)
    }
  });
  if (exitCode !== 0 && exitCode !== void 0) {
    vscode13.window.showErrorMessage("NEON-MDF: Nap UART FU2 that bai; xem terminal. Kiem tra boot mode 100, RESET va baud ROMCODE.");
  }
  return exitCode;
}
async function runFlashStm32Bootloader(state, folder) {
  if (!state.port) {
    vscode13.window.showWarningMessage(
      'NEON-MDF: chua chon cong serial - bam "NEON-MDF: Chon Cong Serial" truoc khi flash qua UART.'
    );
    return void 0;
  }
  if (!await checkOpenocdCfg(state, folder)) {
    return void 0;
  }
  const proceed = await vscode13.window.showInformationMessage(
    "NEON-MDF: Flash qua UART can THAO TAC TAY - Noi day PB2 -> GND + GIU nut B0 tren board (KHONG can bam Reset - tu dong reset qua SWD). Giu nguyen ca 2 cho toi khi thay lenh chay xong.",
    "Da san sang, Tiep tuc",
    "Huy"
  );
  if (proceed !== "Da san sang, Tiep tuc") {
    return void 0;
  }
  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: flash (${state.target}, UART System Bootloader)`,
    preset: state.target,
    cmakeTarget: "flash_uart_stm32",
    cwd: folder,
    // Cong THAT nguoi dung da chon - xem comment o dau ham. Thieu dong nay
    // truoc day (bug that) khien flash_uart_stm32 luon dung gia tri mac
    // dinh hard-code trong CMake cache, khong phai cong that cua board.
    env: { NEON_MDF_SERIAL_PORT: state.port }
  });
  if (exitCode !== 0) {
    vscode13.window.showErrorMessage(
      `NEON-MDF: flash qua UART System Bootloader that bai (target ${state.target}) - xem terminal. Kiem tra da noi PB2->GND + giu B0 dung luc reset chua.`
    );
  } else {
    vscode13.window.showInformationMessage(
      "NEON-MDF: Flash thanh cong - nho THA nut B0 + RUT day PB2->GND, roi reset lai de chay app that."
    );
  }
  return exitCode;
}

// src/commands/buildFlashMonitor.ts
var vscode14 = __toESM(require("vscode"));
async function buildFlashMonitorCommand(state, monitor) {
  const folder = vscode14.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode14.window.showWarningMessage("NEON-MDF: chua mo workspace folder nao.");
    return;
  }
  if (!state.target) {
    vscode14.window.showWarningMessage(
      'NEON-MDF: chua chon target - bam vao "chon target" tren status bar truoc.'
    );
    return;
  }
  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode14.window.showErrorMessage(`NEON-MDF: configure that bai (target ${state.target}) - xem terminal.`);
    return;
  }
  if (!state.port) {
    vscode14.window.showWarningMessage(
      "NEON-MDF: chua chon cong serial - can cho ca flash (duong UART) lan serial monitor."
    );
    return;
  }
  await monitor.close();
  let exitCode;
  if (state.transport === "jtag") {
    exitCode = await runFlashCombined(state, folder.uri);
  } else {
    exitCode = await runFlashUart(state, folder.uri);
    if (exitCode === 0 && state.target !== "FU2") {
      const proceed = await vscode14.window.showInformationMessage(
        "NEON-MDF: Da THA nut B0 + RUT day PB2->GND + reset lai chua? App CHUA CHAY cho toi khi lam xong buoc nay.",
        "Da xong, Mo Monitor",
        "Bo qua Monitor"
      );
      if (proceed !== "Da xong, Mo Monitor") {
        return;
      }
    }
  }
  if (exitCode === void 0) {
    return;
  }
  if (exitCode !== 0) {
    return;
  }
  if (state.target === "FU2") {
    await monitor.open(state);
    return;
  }
  await monitor.close();
  const monitorExitCode = await runCMakeTarget({
    taskName: `NEON-MDF: build + flash + log (${state.target})`,
    preset: state.target,
    cmakeTarget: "serial_monitor",
    cwd: folder.uri,
    env: { NEON_MDF_SERIAL_PORT: state.port },
    background: true
  });
  if (monitorExitCode !== 0) {
    vscode14.window.showErrorMessage(`NEON-MDF: mo serial monitor that bai (target ${state.target}) - xem terminal.`);
  }
}

// src/commands/debug.ts
var vscode15 = __toESM(require("vscode"));

// src/debugArch.ts
var CORTEX_M_PROFILE = {
  debugType: "cortex-debug",
  // Bat DEMCR.VC_CORERESET (bit0 cua 0xE000EDFC, dac ta ARMv7-M) TRUOC khi
  // debug adapter tu lam reset - day la co che PHAN CUNG THAT cua chinh
  // CPU, khong phu thuoc polling/timing cua openocd: CPU se TU DONG vao
  // Debug state NGAY luc lay reset vector, TRUOC KHI chay lenh nao cua
  // Reset_Handler - bat ke reset duoc kich boi co che nao (SRST that hay
  // AIRCR phan mem). Can vi cortex-debug (servertype openocd) mac dinh
  // dung `monitor reset halt` - CHINH LA co che reset da xac nhan KHONG
  // DANG TIN CAY tren combo ST-Link clone + board WeAct nay (cung ly do vi
  // sao flash_jtag/flash_uart_stm32 dung AIRCR.SYSRESETREQ thay vi
  // `reset run` chuan, xem cmake/flash_targets.cmake). Da verify THAT: 4/4
  // lan lap lai voi dung lenh `reset halt` cua openocd (khong phai AIRCR
  // tu viet) cho ket qua GIONG HET nhau (SP = dinh RAM nguyen ven, chua he
  // dung stack - bang chung halt dung luc, truoc khi chay bat ky lenh nao).
  preLaunchCommands: ["monitor halt", "monitor mww 0xE000EDFC 0x00000001"],
  // Tat lai VC_CORERESET sau do (khong anh huong halt da xay ra - chi la
  // thanh ghi dieu khien cho LAN reset TIEP THEO) de khong lam anh huong
  // cac lan reset thu cong khac (vd flash_jtag/flash_uart_stm32 chay ben
  // ngoai VS Code) sau khi phien debug ket thuc.
  postLaunchCommands: ["monitor mww 0xE000EDFC 0x00000000"]
};
function resolveDebugArchProfile(family) {
  if (family === "cortex-m") {
    return CORTEX_M_PROFILE;
  }
  throw new Error(
    `debugArch: chua co DebugArchProfile cho family "${family}" - them 1 profile moi vao debugArch.ts (xem TODO trong resolveDebugArchProfile()).`
  );
}

// src/commands/debug.ts
async function fileExists5(uri) {
  try {
    await vscode15.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}
async function readDebugTargetsManifest(buildDir) {
  const manifestUri = vscode15.Uri.joinPath(buildDir, "neon_mdf_debug_targets.txt");
  if (!await fileExists5(manifestUri)) {
    return null;
  }
  const raw = await vscode15.workspace.fs.readFile(manifestUri);
  const text = Buffer.from(raw).toString("utf-8");
  const manifest = { ota: /* @__PURE__ */ new Set() };
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
async function debugCommand(state) {
  const folder = vscode15.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode15.window.showWarningMessage("NEON-MDF: chua mo workspace folder nao.");
    return;
  }
  if (!state.target) {
    vscode15.window.showWarningMessage(
      'NEON-MDF: chua chon target - bam vao "chon target" tren status bar truoc.'
    );
    return;
  }
  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode15.window.showErrorMessage(`NEON-MDF: configure that bai (target ${state.target}) - xem terminal.`);
    return;
  }
  const buildDir = buildDirFor(folder.uri, state.target);
  const executables = await readExecutables(buildDir);
  if (executables.length === 0) {
    vscode15.window.showWarningMessage(
      `NEON-MDF: khong tim thay executable nao trong build dir cua target ${state.target} (da configure nhung CMake File API khong tra ve target EXECUTABLE nao).`
    );
    return;
  }
  let chosen;
  if (executables.length === 1) {
    chosen = executables[0];
  } else {
    const picked = await vscode15.window.showQuickPick(
      executables.map((e) => ({ label: e.name, exe: e })),
      { placeHolder: "Chon executable de debug" }
    );
    if (!picked) {
      return;
    }
    chosen = picked.exe;
  }
  const openocdCfg = vscode15.Uri.joinPath(
    resolveSdkPathUri(folder.uri),
    "cmake",
    "openocd",
    `${state.target}_openocd.cfg`
  );
  if (!await fileExists5(openocdCfg)) {
    vscode15.window.showErrorMessage(`NEON-MDF: khong tim thay file openocd cfg: ${openocdCfg.fsPath}`);
    return;
  }
  const svdFile = vscode15.Uri.joinPath(
    resolveSdkPathUri(folder.uri),
    "mcu-description",
    "generated",
    `${state.target}.svd`
  );
  const hasSvd = await fileExists5(svdFile);
  const manifest = await readDebugTargetsManifest(buildDir);
  let flashTargetName;
  if (manifest) {
    if (manifest.ota.has(chosen.name)) {
      flashTargetName = "flash_ota";
    } else if (chosen.name === manifest.factory || chosen.name === manifest.bootloader) {
      flashTargetName = "flash_jtag";
    } else {
      vscode15.window.showErrorMessage(
        `NEON-MDF: executable "${chosen.name}" khong duoc gan flash target tu dong nao (khong phai bootloader/factory/OTA theo CMakeLists.txt cua app nay) - khong the debug qua "NEON-MDF: Debug" vi khong biet phai flash gi truoc. Xem CMakeLists.txt cua app de flash/test thu cong (vd ban "_bad" cua examples/OTA_Demo dung de test rollback, can sua CMake tam thoi - xem comment trong file do).`
      );
      return;
    }
  } else {
    flashTargetName = chosen.name.endsWith("_ota") ? "flash_ota" : "flash_jtag";
  }
  const flashExitCode = await runCMakeTarget({
    taskName: `NEON-MDF: flash truoc khi Debug (${flashTargetName})`,
    preset: state.target,
    cmakeTarget: flashTargetName,
    cwd: folder.uri
  });
  if (flashExitCode !== 0) {
    vscode15.window.showErrorMessage(
      `NEON-MDF: flash truoc khi Debug that bai (target CMake "${flashTargetName}") - xem terminal. Debug bi huy.`
    );
    return;
  }
  const archProfile = resolveDebugArchProfile("cortex-m");
  const config = {
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
    postLaunchCommands: archProfile.postLaunchCommands
  };
  if (hasSvd) {
    config.svdFile = svdFile.fsPath;
  }
  const started = await vscode15.debug.startDebugging(folder, config);
  if (!started) {
    vscode15.window.showErrorMessage(
      'NEON-MDF: khong khoi dong duoc phien debug - kiem tra extension "cortex-debug" (marus25.cortex-debug) da cai chua.'
    );
  }
}

// src/commands/menuconfig.ts
var vscode16 = __toESM(require("vscode"));
async function menuconfigCommand(state) {
  const folder = vscode16.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode16.window.showWarningMessage("NEON-MDF: chua mo workspace folder nao.");
    return;
  }
  if (!state.target) {
    vscode16.window.showWarningMessage(
      'NEON-MDF: chua chon target - bam vao "chon target" tren status bar truoc.'
    );
    return;
  }
  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode16.window.showErrorMessage(`NEON-MDF: configure that bai (target ${state.target}) - xem terminal.`);
    return;
  }
  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: menuconfig (${state.target})`,
    preset: state.target,
    cmakeTarget: "menuconfig",
    cwd: folder.uri
  });
  if (exitCode !== 0) {
    vscode16.window.showErrorMessage(`NEON-MDF: menuconfig that bai (target ${state.target}) - xem terminal.`);
  }
}

// src/commands/createApp.ts
var vscode17 = __toESM(require("vscode"));
var APP_NAME_PATTERN = /^[A-Za-z_][A-Za-z0-9_]*$/;
async function pathExists(uri) {
  try {
    await vscode17.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}
async function resolveExamplesDir(openedFolder) {
  const childExamples = vscode17.Uri.joinPath(openedFolder, "examples");
  const childSdk = vscode17.Uri.joinPath(openedFolder, "neon-mdf");
  if (await pathExists(childExamples) && await looksLikeSdkRoot(childSdk)) {
    return childExamples;
  }
  return vscode17.Uri.joinPath(openedFolder, "..");
}
async function copyTemplateDir(src, dest, tokens) {
  await vscode17.workspace.fs.createDirectory(dest);
  const entries = await vscode17.workspace.fs.readDirectory(src);
  for (const [name, type] of entries) {
    const srcChild = vscode17.Uri.joinPath(src, name);
    const destChild = vscode17.Uri.joinPath(dest, name);
    if (type === vscode17.FileType.Directory) {
      await copyTemplateDir(srcChild, destChild, tokens);
    } else {
      const bytes = await vscode17.workspace.fs.readFile(srcChild);
      let text = Buffer.from(bytes).toString("utf8");
      for (const [token, value] of Object.entries(tokens)) {
        text = text.split(token).join(value);
      }
      await vscode17.workspace.fs.writeFile(destChild, Buffer.from(text, "utf8"));
    }
  }
}
async function createAppCommand() {
  const folder = vscode17.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode17.window.showWarningMessage(
      "NEON-MDF: chua mo workspace folder nao - mo thu muc goc repo (neon_mdf/) hoac 1 app vi du (vd examples/Blinky/) truoc."
    );
    return;
  }
  const appName = await vscode17.window.showInputBox({
    prompt: "Ten app moi (dung lam ten thu muc + ten CMake target - chi chu/so/underscore, khong bat dau bang so)",
    placeHolder: "vd my_app",
    validateInput: (value) => {
      if (!value) {
        return "Ten app khong duoc de trong";
      }
      if (!APP_NAME_PATTERN.test(value)) {
        return "Chi cho phep chu/so/underscore, khong bat dau bang so (vd my_app)";
      }
      return void 0;
    }
  });
  if (!appName) {
    return;
  }
  const templateChoice = await vscode17.window.showQuickPick(
    [
      {
        label: "$(circuit-board) App thuong (bare-metal)",
        description: "superloop don gian, khong FreeRTOS",
        templateFolder: "templates"
      },
      {
        label: "$(list-tree) App dung FreeRTOS",
        description: "1 task + scheduler, xem examples/FreeRTOS_Blinky/ cho vi du day du hon",
        templateFolder: "templates_freertos"
      }
    ],
    { placeHolder: "Tao app kieu gi?" }
  );
  if (!templateChoice) {
    return;
  }
  const examplesDir = await resolveExamplesDir(folder.uri);
  const defaultDest = vscode17.Uri.joinPath(examplesDir, appName);
  const picked = await vscode17.window.showQuickPick(
    [
      { label: "$(folder) Tao trong examples/ (khuyen dung)", description: defaultDest.fsPath, dest: defaultDest },
      { label: "$(folder-opened) Chon thu muc khac...", description: "duyet thu muc", dest: void 0 }
    ],
    { placeHolder: "Tao app o dau?" }
  );
  if (!picked) {
    return;
  }
  let destDir = picked.dest;
  if (!destDir) {
    const chosen = await vscode17.window.showOpenDialog({
      canSelectFolders: true,
      canSelectFiles: false,
      canSelectMany: false,
      openLabel: "Tao app o day"
    });
    if (!chosen || chosen.length === 0) {
      return;
    }
    destDir = vscode17.Uri.joinPath(chosen[0], appName);
  }
  if (await pathExists(destDir)) {
    vscode17.window.showErrorMessage(`NEON-MDF: thu muc da ton tai: ${destDir.fsPath}`);
    return;
  }
  const sdkDir = resolveSdkPathUri(destDir);
  if (!await looksLikeSdkRoot(sdkDir)) {
    const proceed = await vscode17.window.showWarningMessage(
      `NEON-MDF: khong tim thay neon-mdf SDK tai ${sdkDir.fsPath} (thu muc nay khong giong 1 checkout SDK that). Neu app dang tao o vi tri khong chuan (khong nam duoi examples/), hay chon SDK path rieng qua lenh "NEON-MDF: Chon Duong Dan SDK..." truoc. Van tao app (co the build se loi)?`,
      "Van tao",
      "Huy"
    );
    if (proceed !== "Van tao") {
      return;
    }
  }
  const templatesDir = vscode17.Uri.joinPath(sdkDir, "tools", templateChoice.templateFolder);
  if (!await pathExists(templatesDir)) {
    vscode17.window.showErrorMessage(`NEON-MDF: khong tim thay thu muc template: ${templatesDir.fsPath}`);
    return;
  }
  const relPath = relativePathToSdk(destDir, sdkDir);
  await copyTemplateDir(templatesDir, destDir, {
    __APP_NAME__: appName,
    __NEON_MDF_RELATIVE_PATH__: relPath
  });
  const openNow = await vscode17.window.showInformationMessage(
    `NEON-MDF: da tao app "${appName}" tai ${destDir.fsPath}`,
    "Mo trong cua so moi"
  );
  if (openNow) {
    await vscode17.commands.executeCommand("vscode.openFolder", destDir, { forceNewWindow: true });
  }
}

// src/commands/selectSdkPath.ts
var vscode18 = __toESM(require("vscode"));
var BROWSE_LABEL = "$(folder-opened) Duyet toi thu muc neon-mdf...";
var RESET_LABEL = "$(discard) Dat lai ve tu dong";
async function selectSdkPathCommand() {
  const current = getConfiguredSdkPath();
  const items = [
    { label: BROWSE_LABEL, description: "chon 1 checkout SDK that" },
    {
      label: RESET_LABEL,
      description: "xoa lua chon, dung lai quy uoc duong dan tuong doi (../../neon-mdf)"
    }
  ];
  const placeHolder = current ? `Dang dung: ${current}` : "Dang dung mac dinh (tu dong suy tu vi tri app) - chon de doi";
  const picked = await vscode18.window.showQuickPick(items, { placeHolder });
  if (!picked) {
    return;
  }
  if (picked.label === RESET_LABEL) {
    await setConfiguredSdkPath(void 0);
    vscode18.window.showInformationMessage("NEON-MDF: da dat lai ve duong dan SDK mac dinh (tu dong).");
    return;
  }
  const chosen = await vscode18.window.showOpenDialog({
    canSelectFolders: true,
    canSelectFiles: false,
    canSelectMany: false,
    openLabel: "Chon lam SDK path",
    defaultUri: current ? vscode18.Uri.file(current) : void 0
  });
  if (!chosen || chosen.length === 0) {
    return;
  }
  const dir = chosen[0];
  if (!await looksLikeSdkRoot(dir)) {
    const proceed = await vscode18.window.showWarningMessage(
      `NEON-MDF: ${dir.fsPath} khong giong 1 checkout neon-mdf SDK (khong thay tools/cmake/project.cmake) - van dung duong dan nay?`,
      "Van dung",
      "Huy"
    );
    if (proceed !== "Van dung") {
      return;
    }
  }
  await setConfiguredSdkPath(dir.fsPath);
  vscode18.window.showInformationMessage(`NEON-MDF: da chon SDK path: ${dir.fsPath}`);
}

// src/commands/openMonitor.ts
async function openMonitorCommand(state, monitor) {
  await monitor.open(state);
}

// src/serialMonitor/monitor.ts
var vscode19 = __toESM(require("vscode"));
var import_serialport3 = require("serialport");
var NeonMdfSerialMonitor = class {
  output;
  port;
  opening;
  lineBuf = "";
  constructor() {
    this.output = vscode19.window.createOutputChannel("NEON-MDF Serial Monitor");
  }
  async open(state) {
    if (!state.port) {
      vscode19.window.showWarningMessage(
        'NEON-MDF: chua chon cong serial - bam vao "chon cong" tren status bar truoc.'
      );
      return;
    }
    await this.close();
    this.lineBuf = "";
    this.output.show(true);
    this.output.appendLine(`--- Mo ${state.port} @ ${state.baudRate} 8N1 ---`);
    const sp = new import_serialport3.SerialPort({ path: state.port, baudRate: state.baudRate, autoOpen: false });
    this.port = sp;
    sp.on("data", (chunk) => this.handleData(chunk));
    sp.on("error", (err) => this.output.appendLine(`--- Loi serial: ${err.message} ---`));
    sp.on("close", () => this.output.appendLine("--- Da dong cong ---"));
    this.opening = new Promise((resolve) => {
      sp.open((err) => {
        if (err) {
          this.output.appendLine(`--- Khong mo duoc cong: ${err.message} ---`);
          if (this.port === sp) {
            this.port = void 0;
          }
        }
        resolve();
      });
    });
    await this.opening;
  }
  handleData(chunk) {
    this.lineBuf += chunk.toString("utf8");
    const lines = this.lineBuf.split("\n");
    this.lineBuf = lines.pop() ?? "";
    for (const line of lines) {
      this.output.appendLine(line.replace(/\r$/, ""));
    }
  }
  async close() {
    const port = this.port;
    const opening = this.opening;
    this.port = void 0;
    this.opening = void 0;
    if (!port) {
      return;
    }
    await opening;
    await new Promise((resolve) => {
      if (port.isOpen) {
        port.close(() => resolve());
      } else {
        resolve();
      }
    });
  }
  dispose() {
    void this.close();
    this.output.dispose();
  }
};

// src/extension.ts
function activate(context) {
  console.log("[NEON-MDF Tools] activated");
  const state = new NeonMdfState(context);
  const statusBar = new NeonMdfStatusBar(state);
  const monitor = new NeonMdfSerialMonitor();
  context.subscriptions.push(
    state,
    statusBar,
    monitor,
    vscode20.commands.registerCommand("neonMdf.selectTarget", () => selectTargetCommand(state)),
    vscode20.commands.registerCommand("neonMdf.selectTransport", () => selectTransportCommand(state)),
    vscode20.commands.registerCommand("neonMdf.selectPort", () => selectPortCommand(state)),
    vscode20.commands.registerCommand("neonMdf.build", () => buildCommand(state)),
    vscode20.commands.registerCommand("neonMdf.flash", () => flashCommand(state, monitor)),
    vscode20.commands.registerCommand("neonMdf.buildFlashMonitor", () => buildFlashMonitorCommand(state, monitor)),
    vscode20.commands.registerCommand("neonMdf.debug", () => debugCommand(state)),
    vscode20.commands.registerCommand("neonMdf.menuconfig", () => menuconfigCommand(state)),
    vscode20.commands.registerCommand("neonMdf.createApp", () => createAppCommand()),
    vscode20.commands.registerCommand("neonMdf.selectSdkPath", () => selectSdkPathCommand()),
    vscode20.commands.registerCommand("neonMdf.openMonitor", () => openMonitorCommand(state, monitor))
  );
}
function deactivate() {
}
// Annotate the CommonJS export names for ESM import in node:
0 && (module.exports = {
  activate,
  deactivate
});
//# sourceMappingURL=extension.js.map
