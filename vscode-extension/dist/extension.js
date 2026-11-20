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
  activate: () => activate
});
module.exports = __toCommonJS(extension_exports);
var vscode15 = __toESM(require("vscode"));

// src/application.ts
var path = __toESM(require("node:path"));
var vscode2 = __toESM(require("vscode"));

// src/presets.ts
var vscode = __toESM(require("vscode"));
async function readJsonFile(uri) {
  try {
    const bytes = await vscode.workspace.fs.readFile(uri);
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
  const rootUri = vscode.Uri.joinPath(workspaceFolder, "CMakePresets.json");
  const root = await readJsonFile(rootUri);
  if (!root) {
    return [];
  }
  const files = [root];
  for (const rel of root.include ?? []) {
    const includeUri = vscode.Uri.joinPath(workspaceFolder, rel);
    const fragment = await readJsonFile(includeUri);
    if (fragment) {
      files.push(fragment);
    }
  }
  return collectTargets(files);
}

// src/application.ts
async function discoverApps(folder) {
  const pattern = new vscode2.RelativePattern(folder, "{CMakePresets.json,examples/*/CMakePresets.json}");
  const presets = await vscode2.workspace.findFiles(pattern);
  const apps = await Promise.all(presets.map(async (preset) => {
    const uri = vscode2.Uri.joinPath(preset, "..");
    try {
      const cmake = await vscode2.workspace.fs.readFile(vscode2.Uri.joinPath(uri, "CMakeLists.txt"));
      if (Buffer.from(cmake).toString("utf8").includes("tools/cmake/project.cmake") && (await discoverTargets(uri)).some((target) => target.name === "GT32U2086")) {
        return { uri, folder };
      }
    } catch {
    }
    return void 0;
  }));
  return apps.filter((app) => app !== void 0);
}
async function findApplications() {
  const apps = (await Promise.all((vscode2.workspace.workspaceFolders ?? []).map(discoverApps))).flat();
  return [...new Map(apps.map((app) => [app.uri.toString(), app])).values()];
}
function resolveApplication(apps, selectedUri) {
  const selected = apps.find((app) => app.uri.toString() === selectedUri);
  if (selected) {
    return selected;
  }
  const active = vscode2.window.activeTextEditor?.document.uri;
  const activeApp = active && apps.find((app) => {
    if (app.uri.scheme !== active.scheme || app.uri.authority !== active.authority) {
      return false;
    }
    const relative3 = path.relative(app.uri.fsPath, active.fsPath);
    return relative3 !== ".." && !relative3.startsWith(".." + path.sep) && !path.isAbsolute(relative3);
  });
  return activeApp || (apps.length === 1 ? apps[0] : void 0);
}
async function selectApplication(state, prompt = false) {
  const apps = await findApplications();
  if (apps.length === 0) {
    vscode2.window.showWarningMessage("NEON-MDF: Open an application folder or the NEON-MDF repository.");
    return;
  }
  const current = resolveApplication(apps, state.selectedApplication);
  if (!prompt && current) {
    return current;
  }
  const choices = apps.map((app) => ({
    label: path.basename(app.uri.fsPath),
    description: vscode2.workspace.asRelativePath(app.uri, true),
    app
  })).sort((a, b) => a.label.localeCompare(b.label));
  const choice = await vscode2.window.showQuickPick(choices, { placeHolder: "Select a GT32U2086 application" });
  if (choice) {
    await state.setApplication(choice.app.uri);
    return choice.app;
  }
}

// src/cmakeRunner.ts
var import_node_crypto = require("node:crypto");
var vscode4 = __toESM(require("vscode"));

// src/sdkPath.ts
var path2 = __toESM(require("node:path"));
var vscode3 = __toESM(require("vscode"));
function sdkPathUri(anchor, sdkPath) {
  return anchor.with({ path: vscode3.Uri.file(sdkPath).path, query: "", fragment: "" });
}
async function looksLikeSdkRoot(dir) {
  try {
    const marker = await vscode3.workspace.fs.stat(vscode3.Uri.joinPath(dir, "tools", "cmake", "project.cmake"));
    return (marker.type & vscode3.FileType.File) !== 0;
  } catch {
    return false;
  }
}
async function validateSdkPath(anchor, sdkPath) {
  if (sdkPath && path2.isAbsolute(sdkPath)) {
    const uri = sdkPathUri(anchor, sdkPath);
    if (await looksLikeSdkRoot(uri)) {
      return uri;
    }
  }
}
function configuredSdkPath(anchor, override) {
  return override ?? vscode3.workspace.getConfiguration("neonMdf", anchor).get("sdkPath", "").trim();
}
async function resolveSdkPath(anchor, override) {
  const configured = await validateSdkPath(anchor, configuredSdkPath(anchor, override));
  if (configured) {
    return { uri: configured, automatic: false };
  }
  for (const parts of [["neon-mdf"], ["..", "..", "neon-mdf"]]) {
    const uri = vscode3.Uri.joinPath(anchor, ...parts);
    if (await looksLikeSdkRoot(uri)) {
      return { uri, automatic: true };
    }
  }
  const fromEnv = await validateSdkPath(anchor, process.env.NEON_MDF_PATH);
  if (fromEnv) {
    return { uri: fromEnv, automatic: true };
  }
}
function relativePathToSdk(fromDir, sdkDir) {
  return path2.relative(fromDir.fsPath, sdkDir.fsPath).split(path2.sep).join("/");
}

// src/cmakeRunner.ts
var PRESET = "GT32U2086";
function runTask(app, name, args, sdkDir, env = {}) {
  const id = (0, import_node_crypto.randomUUID)();
  const taskArgs = sdkDir ? args : ["-E", "env", "--unset=NEON_MDF_PATH", "cmake", ...args];
  const task = new vscode4.Task(
    { type: "neon-mdf", id },
    app.folder,
    name,
    "NEON-MDF",
    new vscode4.ProcessExecution("cmake", taskArgs, {
      cwd: app.uri.fsPath,
      env: { ...env, ...sdkDir ? { NEON_MDF_PATH: sdkDir.fsPath } : {} }
    }),
    []
  );
  task.presentationOptions = {
    reveal: vscode4.TaskRevealKind.Always,
    panel: vscode4.TaskPanelKind.Dedicated
  };
  return new Promise((resolve, reject) => {
    const processEnd = vscode4.tasks.onDidEndTaskProcess((event) => {
      if (event.execution.task.definition.id === id) {
        cleanup();
        resolve(event.exitCode);
      }
    });
    const taskEnd = vscode4.tasks.onDidEndTask((event) => {
      if (event.execution.task.definition.id === id) {
        cleanup();
        resolve(void 0);
      }
    });
    function cleanup() {
      processEnd.dispose();
      taskEnd.dispose();
    }
    vscode4.tasks.executeTask(task).then(void 0, (error) => {
      cleanup();
      reject(error);
    });
  });
}
var preservedOptions = /* @__PURE__ */ new Set([
  "Python3_EXECUTABLE",
  "NEON_MDF_PATH",
  "NEON_MDF_PROFILE",
  "NEON_MDF_SECURE_BOOT",
  "NEON_MDF_SIGNING_KEY",
  "NEON_MDF_GT32U2086_APP_VERSION"
]);
async function freshOptions(app, cache, sdkDir) {
  try {
    const content = Buffer.from(await vscode4.workspace.fs.readFile(cache)).toString("utf8");
    const options = [];
    for (const line of content.split(/\r?\n/)) {
      const entry = /^([A-Za-z0-9_]+):([^=]+)=(.*)$/.exec(line);
      if (entry && entry[2] !== "INTERNAL" && preservedOptions.has(entry[1]) && (entry[1] !== "NEON_MDF_PATH" || !sdkDir && await validateSdkPath(app.uri, entry[3]))) {
        options.push(`-D${entry[1]}=${entry[3]}`);
      }
    }
    return options;
  } catch (error) {
    if (error instanceof vscode4.FileSystemError && error.code === "FileNotFound") {
      return [];
    }
    throw error;
  }
}
async function configure(app, mode, sdkPathOverride) {
  const dir = vscode4.Uri.joinPath(app.uri, "build", PRESET);
  const cache = vscode4.Uri.joinPath(dir, "CMakeCache.txt");
  const sdkDir = (await resolveSdkPath(app.uri, sdkPathOverride))?.uri;
  const args = ["--preset", PRESET];
  if (mode === "fresh") {
    args.push(...await freshOptions(app, cache, sdkDir), "--fresh");
  }
  await requireSuccess(runTask(app, "Configure", args, sdkDir), "Configuration");
}
async function runBuild(app, name, sdkPathOverride, target, env) {
  const args = ["--build", "--preset", PRESET];
  if (target) {
    args.push("--target", target);
  }
  const sdkDir = (await resolveSdkPath(app.uri, sdkPathOverride))?.uri;
  await requireSuccess(runTask(app, name, args, sdkDir, env), name);
}
async function requireSuccess(result, action) {
  const exitCode = await result;
  if (exitCode !== 0) {
    throw new Error(exitCode === void 0 ? `${action} ended without a result. Check the task terminal.` : `${action} failed (exit code ${exitCode}). Check the task terminal.`);
  }
}

// src/commands/createApp.ts
var vscode5 = __toESM(require("vscode"));
var APP_NAME_PATTERN = /^[A-Za-z_][A-Za-z0-9_]*$/;
async function pathExists(uri) {
  try {
    await vscode5.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}
async function resolveExamplesDir(openedFolder) {
  const childExamples = vscode5.Uri.joinPath(openedFolder, "examples");
  const childSdk = vscode5.Uri.joinPath(openedFolder, "neon-mdf");
  if (await pathExists(childExamples) && await looksLikeSdkRoot(childSdk)) {
    return childExamples;
  }
  return vscode5.Uri.joinPath(openedFolder, "..");
}
async function copyTemplateDir(src, dest, tokens) {
  await vscode5.workspace.fs.createDirectory(dest);
  const entries = await vscode5.workspace.fs.readDirectory(src);
  for (const [name, type] of entries) {
    const srcChild = vscode5.Uri.joinPath(src, name);
    const destChild = vscode5.Uri.joinPath(dest, name);
    if (type === vscode5.FileType.Directory) {
      await copyTemplateDir(srcChild, destChild, tokens);
    } else {
      const bytes = await vscode5.workspace.fs.readFile(srcChild);
      let text = Buffer.from(bytes).toString("utf8");
      for (const [token, value] of Object.entries(tokens)) {
        text = text.split(token).join(value);
      }
      await vscode5.workspace.fs.writeFile(destChild, Buffer.from(text, "utf8"));
    }
  }
}
async function createAppCommand(sdkPathOverride) {
  const active = vscode5.window.activeTextEditor?.document.uri;
  const folder = (active && vscode5.workspace.getWorkspaceFolder(active)) ?? vscode5.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode5.window.showWarningMessage(
      "NEON-MDF: No workspace folder is open. Open the repository root (neon_mdf/) or an application such as examples/Blinky/."
    );
    return;
  }
  const appName = await vscode5.window.showInputBox({
    prompt: "App name, used for the folder and CMake target: letters, digits and underscores; must not start with a digit",
    placeHolder: "e.g. my_app",
    validateInput: (value) => {
      if (!value) {
        return "Enter an application name.";
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
  const templateChoice = await vscode5.window.showQuickPick(
    [
      {
        label: "$(circuit-board) Bare-metal app",
        description: "Main loop without FreeRTOS",
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
  const defaultDest = vscode5.Uri.joinPath(examplesDir, appName);
  const picked = await vscode5.window.showQuickPick(
    [
      { label: "$(folder) Use suggested location", description: defaultDest.fsPath, dest: defaultDest },
      { label: "$(folder-opened) Select another folder...", description: "Browse for a destination directory", dest: void 0 }
    ],
    { placeHolder: "Choose where to create the application" }
  );
  if (!picked) {
    return;
  }
  let destDir = picked.dest;
  if (!destDir) {
    const chosen = await vscode5.window.showOpenDialog({
      canSelectFolders: true,
      canSelectFiles: false,
      canSelectMany: false,
      openLabel: "Create app here"
    });
    if (!chosen || chosen.length === 0) {
      return;
    }
    destDir = vscode5.Uri.joinPath(chosen[0], appName);
  }
  if (await pathExists(destDir)) {
    vscode5.window.showErrorMessage(`NEON-MDF: directory already exists: ${destDir.fsPath}`);
    return;
  }
  const sdkDir = (await resolveSdkPath(folder.uri, sdkPathOverride))?.uri;
  if (!sdkDir) {
    vscode5.window.showErrorMessage(
      'NEON-MDF: No SDK found. Run "NEON-MDF: Select SDK Path..." and choose the SDK directory.'
    );
    return;
  }
  const templatesDir = vscode5.Uri.joinPath(sdkDir, "tools", templateChoice.templateFolder);
  if (!await pathExists(templatesDir)) {
    vscode5.window.showErrorMessage(`NEON-MDF: template directory not found: ${templatesDir.fsPath}`);
    return;
  }
  const relPath = relativePathToSdk(destDir, sdkDir);
  await copyTemplateDir(templatesDir, destDir, {
    __APP_NAME__: appName,
    __NEON_MDF_RELATIVE_PATH__: relPath
  });
  const openNow = await vscode5.window.showInformationMessage(
    `NEON-MDF: created app "${appName}" at ${destDir.fsPath}`,
    "Open Application"
  );
  if (openNow) {
    await vscode5.commands.executeCommand("vscode.openFolder", destDir, { forceNewWindow: true });
  }
}

// src/commands/debug.ts
var vscode6 = __toESM(require("vscode"));
async function debugCommand(app) {
  const configurations = vscode6.workspace.getConfiguration("launch", app.folder.uri).get("configurations", []).filter((config2) => config2.type === "cortex-debug" && config2.request === "attach");
  if (configurations.length === 0) {
    throw new Error("Add a Cortex-Debug attach configuration to .vscode/launch.json. See guide/VSCode.md for an example.");
  }
  if (!vscode6.extensions.getExtension("marus25.cortex-debug")) {
    throw new Error("Install Cortex-Debug (marus25.cortex-debug) to attach to the board.");
  }
  const config = configurations.length === 1 ? configurations[0] : (await vscode6.window.showQuickPick(
    configurations.map((entry) => ({ label: entry.name, config: entry })),
    { placeHolder: "Select a Cortex-Debug configuration" }
  ))?.config;
  if (config && !await vscode6.debug.startDebugging(app.folder, config.name)) {
    throw new Error("Debug could not start. Check the Cortex-Debug output and the board's JTAG configuration.");
  }
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

// src/commands/selectPort.ts
var MANUAL_ENTRY = "$(edit) Enter serial port...";
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
    ...ports.map((p) => ({
      label: p.path,
      description: p.manufacturer
    })),
    { label: MANUAL_ENTRY, description: "type serial port" }
  ];
  const picked = await vscode7.window.showQuickPick(items, {
    placeHolder: ports.length > 0 ? "select serial port" : "No ports found; enter the serial port manually"
  });
  if (!picked) {
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
    port = typed.trim();
    if (!port) {
      return;
    }
  }
  await state.setPort(port);
}

// src/commands/selectSdkPath.ts
var vscode8 = __toESM(require("vscode"));
async function selectSdkPathCommand(state) {
  const app = resolveApplication(await findApplications(), state.selectedApplication);
  const anchor = app?.uri ?? vscode8.workspace.workspaceFolders?.[0]?.uri;
  if (!anchor) {
    vscode8.window.showWarningMessage("NEON-MDF: Open a workspace before selecting its SDK.");
    return;
  }
  const current = await resolveSdkPath(anchor, state.sdkPathOverride);
  const items = [
    { label: "$(folder-opened) Browse for the SDK...", value: "browse" },
    { label: "$(discard) Reset to automatic", value: "auto" }
  ];
  const picked = await vscode8.window.showQuickPick(items, {
    placeHolder: current ? `Current SDK: ${current.uri.fsPath}` : "Choose a NEON-MDF SDK directory"
  });
  if (!picked) {
    return;
  }
  if (picked.value === "auto") {
    await state.setSdkPath("");
    return;
  }
  const selected = await vscode8.window.showOpenDialog({
    canSelectFolders: true,
    canSelectFiles: false,
    canSelectMany: false,
    openLabel: "Select SDK Path",
    defaultUri: current?.uri ?? anchor
  });
  const dir = selected?.[0];
  if (!dir) {
    return;
  }
  if (dir.scheme !== anchor.scheme || dir.authority !== anchor.authority || !await looksLikeSdkRoot(dir)) {
    vscode8.window.showErrorMessage("NEON-MDF: Choose a directory on the workspace host that contains tools/cmake/project.cmake.");
    return;
  }
  await state.setSdkPath(dir.fsPath);
}

// src/commands/selectTarget.ts
var vscode9 = __toESM(require("vscode"));
async function selectTargetCommand(state) {
  const app = await selectApplication(state);
  if (!app) {
    return;
  }
  const targets = (await discoverTargets(app.uri)).filter((target) => target.name === "GT32U2086");
  if (targets.length === 0) {
    vscode9.window.showWarningMessage(
      "NEON-MDF: This application has no GT32U2086 CMake preset."
    );
    return;
  }
  const picked = await vscode9.window.showQuickPick(
    targets.map((t) => ({ label: t.displayName, description: t.name })),
    { placeHolder: "select target (read from CMakePresets.json)" }
  );
  if (!picked) {
    return;
  }
  await state.setTarget(picked.description ?? picked.label);
}

// src/commands/selectTransport.ts
var vscode10 = __toESM(require("vscode"));
async function selectTransportCommand(state) {
  await state.setTransport("uart");
  vscode10.window.showInformationMessage(
    "NEON-MDF: GT32U2086 uses ROMCODE UART. Select boot mode 100 and press RESET when the loader is ready."
  );
}

// src/serialMonitor/monitor.ts
var import_node_string_decoder = require("node:string_decoder");
var import_serialport2 = require("serialport");
var vscode11 = __toESM(require("vscode"));
var NeonMdfSerialMonitor = class {
  session;
  get isOpen() {
    return this.session?.port.isOpen ?? false;
  }
  async open(path3, baudRate) {
    if (!Number.isInteger(baudRate) || baudRate <= 0) {
      throw new Error("Set neonMdf.baudRate to a positive integer.");
    }
    if (this.session?.port.path === path3 && this.session.port.baudRate === baudRate && this.isOpen) {
      this.session.terminal.show();
      return;
    }
    await this.close();
    const output = new vscode11.EventEmitter();
    const port = new import_serialport2.SerialPort({ path: path3, baudRate, autoOpen: false });
    const decoder = new import_node_string_decoder.StringDecoder("utf8");
    let previousCR = false;
    let closing;
    const disconnect = () => closing ??= new Promise((resolve, reject) => {
      if (port.isOpen) {
        port.close((error) => error && port.isOpen ? reject(error) : resolve());
      } else {
        resolve();
      }
    }).catch((error) => {
      closing = void 0;
      throw error;
    });
    port.on("error", (error) => output.fire(`\r
[Serial error] ${error.message}\r
`));
    port.on("close", () => output.fire("\r\n[Serial port closed]\r\n"));
    try {
      await new Promise((resolve, reject) => {
        port.open((error) => error ? reject(error) : resolve());
      });
    } catch (error) {
      output.dispose();
      throw error;
    }
    const terminal = vscode11.window.createTerminal({
      name: "NEON-MDF Serial Monitor",
      pty: {
        onDidWrite: output.event,
        open: () => {
          output.fire(`[Connected to ${path3} at ${baudRate} baud, 8N1]\r
`);
          port.on("data", (chunk) => {
            const data = decoder.write(chunk);
            if (data.length === 0) {
              return;
            }
            const normalized = ((previousCR ? "\r" : "") + data).replace(/(?<!\r)\n/g, "\r\n");
            output.fire(previousCR ? normalized.slice(1) : normalized);
            previousCR = data.endsWith("\r");
          });
        },
        handleInput: (data) => {
          if (port.isOpen) {
            port.write(data, (error) => {
              if (error) {
                output.fire(`\r
[Serial write failed] ${error.message}\r
`);
              }
            });
          }
        },
        close: () => {
          output.dispose();
          void disconnect().catch((error) => {
            vscode11.window.showErrorMessage(`NEON-MDF: Could not release ${path3}. ${String(error)}`);
          });
        }
      }
    });
    this.session = { port, terminal, disconnect };
    terminal.show();
  }
  async close() {
    const session = this.session;
    if (session) {
      await session.disconnect();
      this.session = void 0;
      session.terminal.dispose();
    }
  }
  dispose() {
    void this.close().catch((error) => {
      vscode11.window.showErrorMessage(`NEON-MDF: Could not release the serial port. ${String(error)}`);
    });
  }
};

// src/state.ts
var vscode12 = __toESM(require("vscode"));
var PORT_STATE_KEY = "neonMdf.selectedPort";
var SDK_PATH_STATE_KEY = "neonMdf.workspaceSdkPath";
var APPLICATION_STATE_KEY = "neonMdf.selectedApplication";
var NeonMdfState = class {
  constructor(context) {
    this.context = context;
  }
  change = new vscode12.EventEmitter();
  onDidChange = this.change.event;
  uploadStatus = "Idle";
  get selectedApplication() {
    return this.context.workspaceState.get(APPLICATION_STATE_KEY);
  }
  async setApplication(uri) {
    await this.context.workspaceState.update(APPLICATION_STATE_KEY, uri.toString());
    this.change.fire();
  }
  get sdkPathOverride() {
    return this.context.workspaceState.get(SDK_PATH_STATE_KEY);
  }
  async setSdkPath(path3) {
    await this.context.workspaceState.update(SDK_PATH_STATE_KEY, path3);
    this.change.fire();
  }
  get flashStatus() {
    return this.uploadStatus;
  }
  setFlashStatus(status) {
    this.uploadStatus = status;
    this.change.fire();
  }
  get target() {
    return "GT32U2086";
  }
  async setTarget(target) {
    await vscode12.workspace.getConfiguration("neonMdf").update("target", target, vscode12.ConfigurationTarget.Workspace);
    this.change.fire();
  }
  get transport() {
    return "uart";
  }
  async setTransport(transport) {
    await vscode12.workspace.getConfiguration("neonMdf").update("transport", transport, vscode12.ConfigurationTarget.Workspace);
    this.change.fire();
  }
  get baudRate() {
    return vscode12.workspace.getConfiguration("neonMdf").get("baudRate", 19200);
  }
  get flashBaudRate() {
    return vscode12.workspace.getConfiguration("neonMdf").get("gt32u2086FlashBaudRate", 19200);
  }
  get port() {
    return this.context.workspaceState.get(PORT_STATE_KEY);
  }
  async setPort(port) {
    await this.context.workspaceState.update(PORT_STATE_KEY, port);
    this.change.fire();
  }
  dispose() {
    this.change.dispose();
  }
};

// src/statusBar.ts
var vscode13 = __toESM(require("vscode"));
function registerStatusBar(context, state) {
  const actions = [
    ["selectPort", "$(plug) Select Port", "Select the GT32U2086 UART port"],
    ["openMonitor", "$(terminal) Monitor", "Open the interactive UART monitor"],
    ["build", "$(tools) Build", "Build the GT32U2086 application"],
    ["flash", "$(arrow-down) Flash", "Program the firmware through ROMCODE UART"],
    ["menuconfig", "$(settings-gear) Menuconfig", "Configure application and driver options"],
    ["debug", "$(debug-alt) Debug", "Attach using the workspace's Cortex-Debug configuration"],
    ["selectTarget", "$(circuit-board) GT32U2086", "Select the target preset"],
    ["selectTransport", "$(plug) UART", "Select the programming transport"]
  ];
  const items = actions.map(([name, text, tooltip], index) => {
    const item = vscode13.window.createStatusBarItem(`neonMdf.${name}`, vscode13.StatusBarAlignment.Left, 100 - index);
    item.command = `neonMdf.${name}`;
    item.text = text;
    item.tooltip = tooltip;
    item.show();
    return item;
  });
  const updateSelections = () => {
    items[0].text = state.port ? `$(plug) ${state.port}` : "$(plug) Select Port";
    items[6].text = `$(circuit-board) ${state.target}`;
    items[7].text = `$(plug) ${state.transport.toUpperCase()}`;
  };
  context.subscriptions.push(...items, state.onDidChange(updateSelections));
  updateSelections();
}

// src/sidebar.ts
var import_node_path = require("node:path");
var vscode14 = __toESM(require("vscode"));
var groups = ["Project", "Device", "Actions"];
var Section = class extends vscode14.TreeItem {
  constructor(group) {
    super(group, vscode14.TreeItemCollapsibleState.Expanded);
    this.group = group;
    this.id = group;
  }
};
function row(label, icon, command, description) {
  const item = new vscode14.TreeItem(label);
  item.iconPath = new vscode14.ThemeIcon(icon);
  item.description = description;
  if (command) {
    item.command = { command: `neonMdf.${command}`, title: label };
  }
  return item;
}
function baudRow(label, baud, setting) {
  const item = row(label, "settings-gear", void 0, String(baud));
  item.command = {
    command: "workbench.action.openSettings",
    title: label,
    arguments: [`@id:neonMdf.${setting}`]
  };
  return item;
}
function registerSidebar(context, state) {
  const changed = new vscode14.EventEmitter();
  context.subscriptions.push(
    changed,
    state.onDidChange(() => changed.fire()),
    vscode14.window.onDidChangeActiveTextEditor(() => changed.fire()),
    vscode14.workspace.onDidChangeWorkspaceFolders(() => changed.fire()),
    vscode14.workspace.onDidChangeConfiguration((event) => {
      if (event.affectsConfiguration("neonMdf")) {
        changed.fire();
      }
    }),
    vscode14.window.createTreeView("neonMdf.controlPanel", {
      showCollapseAll: false,
      treeDataProvider: {
        onDidChangeTreeData: changed.event,
        getTreeItem: (item) => item,
        getChildren: async (item) => {
          if (!item) {
            return groups.map((group) => new Section(group));
          }
          if (!(item instanceof Section)) {
            return [];
          }
          switch (item.group) {
            case "Project": {
              const app = resolveApplication(await findApplications(), state.selectedApplication);
              const application = row(
                "Application",
                "folder-opened",
                "selectApp",
                app ? (0, import_node_path.basename)(app.uri.fsPath) : "Choose application"
              );
              application.tooltip = app?.uri.fsPath ?? "Select a GT32U2086 application";
              const anchor = app?.uri ?? vscode14.workspace.workspaceFolders?.[0]?.uri;
              const resolved = anchor && await resolveSdkPath(anchor, state.sdkPathOverride);
              const sdk = row(
                "SDK Path",
                "settings-gear",
                "selectSdkPath",
                resolved && !resolved.automatic ? (0, import_node_path.basename)(resolved.uri.fsPath) : "Auto"
              );
              sdk.tooltip = resolved?.uri.fsPath ?? "Select an SDK directory on the workspace host";
              return [
                application,
                row("Target", "circuit-board", "selectTarget", state.target),
                sdk,
                row("Create New App\u2026", "add", "createApp")
              ];
            }
            case "Device":
              return [
                row("Transport", "plug", "selectTransport", state.transport.toUpperCase()),
                row("Serial Port", "plug", "selectPort", state.port ?? "Choose port"),
                baudRow("Flash Baud", state.flashBaudRate, "gt32u2086FlashBaudRate"),
                baudRow("Monitor Baud", state.baudRate, "baudRate"),
                row("UART Upload", "info", void 0, state.flashStatus)
              ];
            case "Actions": {
              const flash = row("Flash via UART", "arrow-down", "flash");
              flash.tooltip = "Set boot mode 100. Press RESET when the terminal reports that UART is ready.";
              return [
                row("Build", "tools", "build"),
                flash,
                row("Build + Flash + Log", "run-all", "buildFlashMonitor"),
                row("Serial Monitor", "terminal", "openMonitor"),
                row("Menuconfig", "settings-gear", "menuconfig"),
                row("Debug", "debug-alt", "debug")
              ];
            }
          }
        }
      }
    })
  );
}

// src/extension.ts
async function activate(context) {
  const state = new NeonMdfState(context);
  const monitor = new NeonMdfSerialMonitor();
  let busy = false;
  context.subscriptions.push(state, monitor);
  function register(name, action) {
    context.subscriptions.push(vscode15.commands.registerCommand(`neonMdf.${name}`, async () => {
      if (busy) {
        vscode15.window.showInformationMessage("NEON-MDF: Wait for the current operation to finish or stop its task.");
        return;
      }
      busy = true;
      try {
        await action();
      } catch (error) {
        vscode15.window.showErrorMessage(`NEON-MDF: ${error instanceof Error ? error.message : String(error)}`);
      } finally {
        busy = false;
      }
    }));
  }
  async function getPort() {
    if (!state.port) {
      await selectPortCommand(state);
    }
    return state.port;
  }
  register("selectPort", async () => {
    const previous = state.port;
    await selectPortCommand(state);
    if (monitor.isOpen && state.port && state.port !== previous) {
      await monitor.open(state.port, state.baudRate);
    }
  });
  register("openMonitor", async () => {
    const port = await getPort();
    if (port) {
      await monitor.open(port, state.baudRate);
    }
  });
  register("selectApp", async () => {
    await selectApplication(state, true);
  });
  register("build", async () => {
    const app = await selectApplication(state);
    if (app) {
      await configure(app, "fresh", state.sdkPathOverride);
      await runBuild(app, "Build", state.sdkPathOverride);
    }
  });
  async function flash(openMonitor) {
    const app = await selectApplication(state);
    if (!app) {
      return;
    }
    const port = await getPort();
    if (!port) {
      return;
    }
    const baudRate = state.flashBaudRate;
    if (!Number.isInteger(baudRate) || baudRate <= 0) {
      throw new Error("Set neonMdf.gt32u2086FlashBaudRate to a positive integer matching ROMCODE.");
    }
    const reopenMonitor = openMonitor || monitor.isOpen;
    state.setFlashStatus("Preparing");
    try {
      await configure(app, "fresh", state.sdkPathOverride);
      await monitor.close();
      state.setFlashStatus("Running UART upload");
      await runBuild(app, "Flash via UART", state.sdkPathOverride, "flash_uart_gt32u2086", {
        NEON_MDF_SERIAL_PORT: port,
        NEON_MDF_SERIAL_BAUD: String(baudRate),
        NEON_MDF_UART_BYTE_GAP_MS: "0",
        NEON_MDF_UART_LOG: "progress"
      });
      state.setFlashStatus("Complete");
    } catch (error) {
      state.setFlashStatus("Failed or cancelled");
      throw error;
    }
    if (reopenMonitor) {
      await monitor.open(port, state.baudRate);
    }
  }
  register("flash", () => flash(false));
  register("buildFlashMonitor", () => flash(true));
  register("createApp", () => createAppCommand(state.sdkPathOverride));
  register("selectSdkPath", () => selectSdkPathCommand(state));
  register("selectTarget", () => selectTargetCommand(state));
  register("selectTransport", () => selectTransportCommand(state));
  register("menuconfig", async () => {
    const app = await selectApplication(state);
    if (app) {
      await configure(app, "fresh", state.sdkPathOverride);
      await runBuild(app, "Menuconfig", state.sdkPathOverride, "menuconfig");
      await configure(app, "refresh", state.sdkPathOverride);
    }
  });
  register("debug", async () => {
    const app = await selectApplication(state);
    if (app) {
      await debugCommand(app);
    }
  });
  registerSidebar(context, state);
  context.subscriptions.push(vscode15.commands.registerCommand(
    "neonMdf.openBoardTools",
    () => vscode15.commands.executeCommand("neonMdf.controlPanel.focus")
  ));
  if ((await findApplications()).length > 0) {
    registerStatusBar(context, state);
  }
}
// Annotate the CommonJS export names for ESM import in node:
0 && (module.exports = {
  activate
});
//# sourceMappingURL=extension.js.map
