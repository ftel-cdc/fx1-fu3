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
    this.targetItem.text = `$(circuit-board) ${this.state.target || "select target"}`;
    this.targetItem.tooltip = "NEON-MDF: Select Target";
    this.transportItem.text = this.state.transport === "jtag" ? "$(circuit-board) JTAG" : "$(plug) UART";
    this.transportItem.tooltip = "NEON-MDF: Flash transport (JTAG: one fast SWD session; UART: System Bootloader, manual preparation required)";
    this.portItem.text = `$(radio-tower) ${this.state.port ?? "no port selected"}`;
    this.portItem.tooltip = "NEON-MDF: Select Serial Port";
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
    vscode4.window.showWarningMessage("NEON-MDF: no workspace folder opened.");
    return;
  }
  const targets = await discoverTargets(folder.uri);
  if (targets.length === 0) {
    vscode4.window.showWarningMessage(
      "NEON-MDF: No targets found in this workspace CMakePresets.json. Open an application folder such as examples/Blinky."
    );
    return;
  }
  const picked = await vscode4.window.showQuickPick(
    targets.map((t) => ({ label: t.displayName, description: t.name })),
    { placeHolder: "select target (read from CMakePresets.json)" }
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
      description: state.target === "FU2" ? "ROMCODE USART update: select boot mode 100 and press RESET when the tool is ready" : "System Memory Bootloader STM32: target flash_uart_stm32, operation B0/PB2 by board",
      value: "uart"
    },
    {
      label: "$(circuit-board) JTAG/SWD",
      description: state.target === "FU2" ? "unsupported: configuration OpenOCD FU2 is a placeholder" : "flash via OpenOCD (target flash_jtag)",
      value: "jtag"
    }
  ];
  const picked = await vscode5.window.showQuickPick(items, { placeHolder: "select transport" });
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
      title: "NEON-MDF: Probing serial ports - reset or power-cycle the board now..."
    },
    async () => {
      const found = await detectBootloaderPort();
      if (!found) {
        vscode6.window.showWarningMessage(
          "NEON-MDF: No bootloader responded within the SYNC window (~9.5s). Reset or power-cycle the board while this command runs, or select a port manually."
        );
        return;
      }
      await state.setPort(found);
      vscode6.window.showInformationMessage(`NEON-MDF: found bootloader on ${found} port.`);
    }
  );
}

// src/commands/selectPort.ts
var MANUAL_ENTRY = "$(edit) enter hand...";
var AUTO_DETECT_ENTRY = "$(zap) auto that (RESET board before)...";
async function selectPortCommand(state) {
  let ports = [];
  try {
    ports = await listPorts();
  } catch (err) {
    vscode7.window.showWarningMessage(
      `NEON-MDF: unable to read serial ports list (${String(err)}) - manual input.`
    );
  }
  const items = [
    { label: AUTO_DETECT_ENTRY, description: "that port is running bootloader (real delivery SYNC/READY)" },
    ...ports.map((p) => ({
      label: p.path,
      description: p.manufacturer
    })),
    { label: MANUAL_ENTRY, description: "type serial port" }
  ];
  const picked = await vscode7.window.showQuickPick(items, {
    placeHolder: ports.length > 0 ? "select serial port" : "no knob found - enter that hand or auto"
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
      prompt: "Serial port, e.g. /dev/cu.usbmodem102, COM3 or /dev/ttyACM0",
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
    // sdkPathEnv() only checks the value if the user has selected a separate SDK path (neonMdf.sdkPath) -
    // it is not necessary to leave in, so that CMakeLists.txt/ settings.json of each app from the
    // specified as before.
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
    vscode12.window.showWarningMessage("NEON-MDF: no workspace folder opened.");
    return;
  }
  if (!state.target) {
    vscode12.window.showWarningMessage(
      "NEON-MDF: No target selected. Use Select Target in the status bar first."
    );
    return;
  }
  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode12.window.showErrorMessage(`NEON-MDF: configure failed (target ${state.target}) - see terminal.`);
    return;
  }
  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: build (${state.target})`,
    preset: state.target,
    cwd: folder.uri
  });
  if (exitCode !== 0) {
    vscode12.window.showErrorMessage(`NEON-MDF: build failed (target ${state.target}).`);
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
    vscode13.window.showErrorMessage(`NEON-MDF: could not find openocd cfg file: ${openocdCfg.fsPath}`);
    return false;
  }
  return true;
}
async function flashCommand(state, monitor) {
  const folder = vscode13.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode13.window.showWarningMessage("NEON-MDF: no workspace folder opened.");
    return;
  }
  if (!state.target) {
    vscode13.window.showWarningMessage(
      "NEON-MDF: No target selected. Use Select Target in the status bar first."
    );
    return;
  }
  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode13.window.showErrorMessage(`NEON-MDF: configure failed (target ${state.target}) - see terminal.`);
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
    vscode13.window.showErrorMessage("NEON-MDF: FU2 OpenOCD does not have a flash configuration. select Transport UART to flash via ROMCODE.");
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
    vscode13.window.showErrorMessage(`NEON-MDF: flash via SWD failed (target ${state.target}) - see terminal.`);
  }
  return exitCode;
}
async function runFlashUart(state, folder) {
  if (state.target !== "FU2") {
    return runFlashStm32Bootloader(state, folder);
  }
  if (!state.port) {
    vscode13.window.showWarningMessage("NEON-MDF: select the serial port before flashing UART FU2.");
    return void 0;
  }
  if (!Number.isInteger(state.flashBaudRate) || state.flashBaudRate <= 0) {
    vscode13.window.showErrorMessage("NEON-MDF: neonMdf.fu2FlashBaudRate must be a ROMCODE matching line integer.");
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
    vscode13.window.showErrorMessage("NEON-MDF: UART FU2 flash failed; see terminal. test boot mode 100, RESET and baud ROMCODE.");
  }
  return exitCode;
}
async function runFlashStm32Bootloader(state, folder) {
  if (!state.port) {
    vscode13.window.showWarningMessage(
      "NEON-MDF: No serial port selected. Use NEON-MDF: Select Serial Port before UART flashing."
    );
    return void 0;
  }
  if (!await checkOpenocdCfg(state, folder)) {
    return void 0;
  }
  const proceed = await vscode13.window.showInformationMessage(
    "NEON-MDF: UART flashing requires manual preparation: connect PB2 to GND and hold B0 on the board. Reset is automatic through SWD. Keep both in place until flashing finishes.",
    "leveled, continue",
    "deactivate"
  );
  if (proceed !== "leveled, continue") {
    return void 0;
  }
  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: flash (${state.target}, UART System Bootloader)`,
    preset: state.target,
    cmakeTarget: "flash_uart_stm32",
    cwd: folder,
    // Pass the actual user-selected port; omitting this previously caused a
    // bug): flash_uart_stm32 always used the hardcoded CMake-cache default instead of the selected board
    // ports.
    env: { NEON_MDF_SERIAL_PORT: state.port }
  });
  if (exitCode !== 0) {
    vscode13.window.showErrorMessage(
      `NEON-MDF: flash via UART System Bootloader failed (target ${state.target}) - see terminal. checked that PB2->GND + keep B0 used for reset.`
    );
  } else {
    vscode13.window.showInformationMessage(
      "NEON-MDF: Flashing succeeded. Release B0, disconnect PB2 from GND, then reset to run the app."
    );
  }
  return exitCode;
}

// src/commands/buildFlashMonitor.ts
var vscode14 = __toESM(require("vscode"));
async function buildFlashMonitorCommand(state, monitor) {
  const folder = vscode14.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode14.window.showWarningMessage("NEON-MDF: no workspace folder opened.");
    return;
  }
  if (!state.target) {
    vscode14.window.showWarningMessage(
      "NEON-MDF: No target selected. Use Select Target in the status bar first."
    );
    return;
  }
  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode14.window.showErrorMessage(`NEON-MDF: configure failed (target ${state.target}) - see terminal.`);
    return;
  }
  if (!state.port) {
    vscode14.window.showWarningMessage(
      "NEON-MDF: not selected serial port - needed for both flash (UART path) times serial monitor."
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
        "NEON-MDF: Have you released B0, disconnected PB2 from GND and reset the board? Complete these steps to start the app.",
        "Done, Open Monitor",
        "ignore Monitor"
      );
      if (proceed !== "Done, Open Monitor") {
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
    vscode14.window.showErrorMessage(`NEON-MDF: Opening serial monitor failed (target ${state.target}) - see terminal.`);
  }
}

// src/commands/debug.ts
var vscode15 = __toESM(require("vscode"));

// src/debugArch.ts
var CORTEX_M_PROFILE = {
  debugType: "cortex-debug",
  // Enable DEMCR.VC_CORERESET (bit0 of 0xE000EDFC, ARMv7-M) before the debug adapter resets the target.
  // This CPU hardware mechanism does not depend on OpenOCD polling or timing:
  // The CPU will automatically enter the Debug state NOW to get the reset vector, before running
  // instruction in Reset_Handler, regardless of whether reset uses physical SRST
  // let AIRCR software). need to use default cortex-debug (servertype openocd) using `monitor reset
  // halt` - this is a confirmed not trusting reset cover on this ST-Link clone + WeAct board combo
  // (same reason why flash_jtag/flash_uart_stm32 uses AIRCR.SYSRESETREQ see because `reset run`
  // standard behavior; see cmake/flash_targets.cmake). Verified in four of four OpenOCD `reset
  // halt` command, rather than an AIRCR write, gave identical results: SP at the top of RAM, before stack
  // a general halt during execution, before any instruction runs).
  preLaunchCommands: ["monitor halt", "monitor mww 0xE000EDFC 0x00000001"],
  // re-disable VC_CORERESET afterwards (not the halt-directed image that was milled out - just the
  // control register for the next reset) so as not to direct the image to other curved message
  // resets outside VS Code, such as flash_jtag/flash_uart_stm32, after the debug session
  // authentication.
  postLaunchCommands: ["monitor mww 0xE000EDFC 0x00000000"]
};
function resolveDebugArchProfile(family) {
  if (family === "cortex-m") {
    return CORTEX_M_PROFILE;
  }
  throw new Error(
    `debugArch: no DebugArchProfile for family "${family}" yet - add 1 profile per debugArch.ts (see TODO in resolveDebugArchProfile()).`
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
    vscode15.window.showWarningMessage("NEON-MDF: no workspace folder opened.");
    return;
  }
  if (!state.target) {
    vscode15.window.showWarningMessage(
      "NEON-MDF: No target selected. Use Select Target in the status bar first."
    );
    return;
  }
  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode15.window.showErrorMessage(`NEON-MDF: configure failed (target ${state.target}) - see terminal.`);
    return;
  }
  const buildDir = buildDirFor(folder.uri, state.target);
  const executables = await readExecutables(buildDir);
  if (executables.length === 0) {
    vscode15.window.showWarningMessage(
      `NEON-MDF: No executable found for target ${state.target}. Configuration completed, but the CMake File API returned no EXECUTABLE target.`
    );
    return;
  }
  let chosen;
  if (executables.length === 1) {
    chosen = executables[0];
  } else {
    const picked = await vscode15.window.showQuickPick(
      executables.map((e) => ({ label: e.name, exe: e })),
      { placeHolder: "select executable to debug" }
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
    vscode15.window.showErrorMessage(`NEON-MDF: could not find openocd cfg file: ${openocdCfg.fsPath}`);
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
        `NEON-MDF: executable "${chosen.name}" not assigned flash target automatically (not bootloader/factory/OTA according to CMakeLists.txt of this app) - cannot debug via "NEON-MDF: Debug" because I don't know what to flash first. See CMakeLists.txt of app to flash/test curved message (e.g. the "_bad" version of examples/OTA_Demo used for testing rollback, need to fix CMake temporarily - see comment in that file).`
      );
      return;
    }
  } else {
    flashTargetName = chosen.name.endsWith("_ota") ? "flash_ota" : "flash_jtag";
  }
  const flashExitCode = await runCMakeTarget({
    taskName: `NEON-MDF: flash before Debug (${flashTargetName})`,
    preset: state.target,
    cmakeTarget: flashTargetName,
    cwd: folder.uri
  });
  if (flashExitCode !== 0) {
    vscode15.window.showErrorMessage(
      `NEON-MDF: flash before debug fails (target CMake "${flashTargetName}") - see terminal. Debug aborted.`
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
    // [] (not false - read source cortex-debug: false still running normal `target-download`, only empty
    // network []/list per instance ignoring download) - already flashed using the above method
    // (flash_jtag/ flash_ota, keep header intact), do not let cortex-debug flash again old time
    // using GDB load, which would erase the image header; see above. This behavior applies to all
    // each live kien (still leaves header/flashmap of this SDK itself, not ARM letter dac) - not
    // male in debugArch.ts.
    loadFiles: [],
    // workaround command reset DAC mail BY KIEN live - see debugArch.ts.
    preLaunchCommands: archProfile.preLaunchCommands,
    postLaunchCommands: archProfile.postLaunchCommands
  };
  if (hasSvd) {
    config.svdFile = svdFile.fsPath;
  }
  const started = await vscode15.debug.startDebugging(folder, config);
  if (!started) {
    vscode15.window.showErrorMessage(
      "NEON-MDF: Could not start the debug session. Check that cortex-debug (marus25.cortex-debug) is enabled."
    );
  }
}

// src/commands/menuconfig.ts
var vscode16 = __toESM(require("vscode"));
async function menuconfigCommand(state) {
  const folder = vscode16.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode16.window.showWarningMessage("NEON-MDF: no workspace folder opened.");
    return;
  }
  if (!state.target) {
    vscode16.window.showWarningMessage(
      "NEON-MDF: No target selected. Use Select Target in the status bar first."
    );
    return;
  }
  const configOk = await ensureConfigured(folder.uri, state.target);
  if (!configOk) {
    vscode16.window.showErrorMessage(`NEON-MDF: configure failed (target ${state.target}) - see terminal.`);
    return;
  }
  const exitCode = await runCMakeTarget({
    taskName: `NEON-MDF: menuconfig (${state.target})`,
    preset: state.target,
    cmakeTarget: "menuconfig",
    cwd: folder.uri
  });
  if (exitCode !== 0) {
    vscode16.window.showErrorMessage(`NEON-MDF: menuconfig failed (target ${state.target}) - see terminal.`);
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
      "NEON-MDF: No workspace folder is open. Open the repository root (neon_mdf/) or an application such as examples/Blinky/."
    );
    return;
  }
  const appName = await vscode17.window.showInputBox({
    prompt: "App name, used for the folder and CMake target: letters, digits and underscores; must not start with a digit",
    placeHolder: "vd my_app",
    validateInput: (value) => {
      if (!value) {
        return "app name not in";
      }
      if (!APP_NAME_PATTERN.test(value)) {
        return "Use letters, digits and underscores; do not start with a digit, for example my_app";
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
        label: "$(circuit-board) Bare-metal app",
        description: "superloop don space, no FreeRTOS",
        templateFolder: "templates"
      },
      {
        label: "$(list-tree) App using FreeRTOS",
        description: "1 task + scheduler, see examples/FreeRTOS_Blinky/ for more detail",
        templateFolder: "templates_freertos"
      }
    ],
    { placeHolder: "What kind of app?" }
  );
  if (!templateChoice) {
    return;
  }
  const examplesDir = await resolveExamplesDir(folder.uri);
  const defaultDest = vscode17.Uri.joinPath(examplesDir, appName);
  const picked = await vscode17.window.showQuickPick(
    [
      { label: "$(folder) created in examples/ (recommended)", description: defaultDest.fsPath, dest: defaultDest },
      { label: "$(folder-opened) select different folder...", description: "duyet directory", dest: void 0 }
    ],
    { placeHolder: "create app at the top?" }
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
      openLabel: "Create app here"
    });
    if (!chosen || chosen.length === 0) {
      return;
    }
    destDir = vscode17.Uri.joinPath(chosen[0], appName);
  }
  if (await pathExists(destDir)) {
    vscode17.window.showErrorMessage(`NEON-MDF: directory already exists: ${destDir.fsPath}`);
    return;
  }
  const sdkDir = resolveSdkPathUri(destDir);
  if (!await looksLikeSdkRoot(sdkDir)) {
    const proceed = await vscode17.window.showWarningMessage(
      `NEON-MDF: can't find neon-mdf SDK at ${sdkDir.fsPath} (this directory is not the same as a real SDK checkout). if the app is creating at an unknown value (not below examples/), select its own SDK path via the command "NEON-MDF: select Dan SDK path..." first. still creating app (build might fail)?`,
      "still create",
      "deactivate"
    );
    if (proceed !== "still create") {
      return;
    }
  }
  const templatesDir = vscode17.Uri.joinPath(sdkDir, "tools", templateChoice.templateFolder);
  if (!await pathExists(templatesDir)) {
    vscode17.window.showErrorMessage(`NEON-MDF: template directory not found: ${templatesDir.fsPath}`);
    return;
  }
  const relPath = relativePathToSdk(destDir, sdkDir);
  await copyTemplateDir(templatesDir, destDir, {
    __APP_NAME__: appName,
    __NEON_MDF_RELATIVE_PATH__: relPath
  });
  const openNow = await vscode17.window.showInformationMessage(
    `NEON-MDF: created app "${appName}" at ${destDir.fsPath}`,
    "open in of each number"
  );
  if (openNow) {
    await vscode17.commands.executeCommand("vscode.openFolder", destDir, { forceNewWindow: true });
  }
}

// src/commands/selectSdkPath.ts
var vscode18 = __toESM(require("vscode"));
var BROWSE_LABEL = "$(folder-opened) Browse my neon-mdf directory...";
var RESET_LABEL = "$(discard) reset to automatic";
async function selectSdkPathCommand() {
  const current = getConfiguredSdkPath();
  const items = [
    { label: BROWSE_LABEL, description: "select 1 real SDK checkout" },
    {
      label: RESET_LABEL,
      description: "select erase, reuse the modified dan convention (../../neon-mdf)"
    }
  ];
  const placeHolder = current ? `in use: ${current}` : "using default (automatically inferred from app value) - select to change";
  const picked = await vscode18.window.showQuickPick(items, { placeHolder });
  if (!picked) {
    return;
  }
  if (picked.label === RESET_LABEL) {
    await setConfiguredSdkPath(void 0);
    vscode18.window.showInformationMessage("NEON-MDF: reset to default SDK path (automatic).");
    return;
  }
  const chosen = await vscode18.window.showOpenDialog({
    canSelectFolders: true,
    canSelectFiles: false,
    canSelectMany: false,
    openLabel: "Select SDK Path",
    defaultUri: current ? vscode18.Uri.file(current) : void 0
  });
  if (!chosen || chosen.length === 0) {
    return;
  }
  const dir = chosen[0];
  if (!await looksLikeSdkRoot(dir)) {
    const proceed = await vscode18.window.showWarningMessage(
      `NEON-MDF: ${dir.fsPath} is not the same as a neon-mdf SDK checkout (no tools/cmake/project.cmake seen) - still use this dan path?`,
      "still use",
      "deactivate"
    );
    if (proceed !== "still use") {
      return;
    }
  }
  await setConfiguredSdkPath(dir.fsPath);
  vscode18.window.showInformationMessage(`NEON-MDF: selected SDK path: ${dir.fsPath}`);
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
        "NEON-MDF: No serial port selected. Use Select Serial Port in the status bar first."
      );
      return;
    }
    await this.close();
    this.lineBuf = "";
    this.output.show(true);
    this.output.appendLine(`--- Open ${state.port} @ ${state.baudRate} 8N1 ---`);
    const sp = new import_serialport3.SerialPort({ path: state.port, baudRate: state.baudRate, autoOpen: false });
    this.port = sp;
    sp.on("data", (chunk) => this.handleData(chunk));
    sp.on("error", (err) => this.output.appendLine(`--- Serial error: ${err.message} ---`));
    sp.on("close", () => this.output.appendLine("--- Port closed ---"));
    this.opening = new Promise((resolve) => {
      sp.open((err) => {
        if (err) {
          this.output.appendLine(`--- Cannot open port: ${err.message} ---`);
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
