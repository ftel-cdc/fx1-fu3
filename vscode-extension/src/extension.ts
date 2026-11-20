import * as vscode from "vscode";
import { findApplications, selectApplication } from "./application";
import { configure, runBuild } from "./cmakeRunner";
import { createAppCommand } from "./commands/createApp";
import { debugCommand } from "./commands/debug";
import { selectPortCommand } from "./commands/selectPort";
import { selectSdkPathCommand } from "./commands/selectSdkPath";
import { selectTargetCommand } from "./commands/selectTarget";
import { selectTransportCommand } from "./commands/selectTransport";
import { NeonMdfSerialMonitor } from "./serialMonitor/monitor";
import { NeonMdfState } from "./state";
import { registerStatusBar } from "./statusBar";
import { registerSidebar } from "./sidebar";

export async function activate(context: vscode.ExtensionContext): Promise<void> {
  const state = new NeonMdfState(context);
  const monitor = new NeonMdfSerialMonitor();
  let busy = false;
  context.subscriptions.push(state, monitor);

  function register(name: string, action: () => Promise<void>): void {
    context.subscriptions.push(vscode.commands.registerCommand(`neonMdf.${name}`, async () => {
      if (busy) {
        vscode.window.showInformationMessage("NEON-MDF: Wait for the current operation to finish or stop its task.");
        return;
      }
      busy = true;
      try {
        await action();
      } catch (error) {
        vscode.window.showErrorMessage(`NEON-MDF: ${error instanceof Error ? error.message : String(error)}`);
      } finally {
        busy = false;
      }
    }));
  }

  async function getPort(): Promise<string | undefined> {
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
      await configure(app, "fresh");
      await runBuild(app, "Build");
    }
  });
  async function flash(openMonitor: boolean): Promise<void> {
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
      await configure(app, "fresh");
      await monitor.close();
      state.setFlashStatus("Running UART upload");
      await runBuild(app, "Flash via UART", "flash_uart_gt32u2086", {
        NEON_MDF_SERIAL_PORT: port,
        NEON_MDF_SERIAL_BAUD: String(baudRate),
        NEON_MDF_UART_BYTE_GAP_MS: "0",
        NEON_MDF_UART_LOG: "progress",
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
  register("createApp", createAppCommand);
  register("selectSdkPath", selectSdkPathCommand);
  register("selectTarget", () => selectTargetCommand(state));
  register("selectTransport", () => selectTransportCommand(state));
  register("menuconfig", async () => {
    const app = await selectApplication(state);
    if (app) {
      await configure(app);
      await runBuild(app, "Menuconfig", "menuconfig");
      await configure(app, "refresh");
    }
  });
  register("debug", async () => {
    const app = await selectApplication(state);
    if (app) {
      await debugCommand(app);
    }
  });

  registerSidebar(context, state);
  context.subscriptions.push(vscode.commands.registerCommand("neonMdf.openBoardTools", () =>
    vscode.commands.executeCommand("neonMdf.controlPanel.focus"),
  ));
  if ((await findApplications()).length > 0) {
    registerStatusBar(context, state);
  }
}
