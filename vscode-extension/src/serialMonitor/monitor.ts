import { StringDecoder } from "node:string_decoder";
import { SerialPort } from "serialport";
import * as vscode from "vscode";

interface MonitorSession {
  port: SerialPort;
  terminal: vscode.Terminal;
  disconnect(): Promise<void>;
}

export class NeonMdfSerialMonitor implements vscode.Disposable {
  private session: MonitorSession | undefined;

  get isOpen(): boolean {
    return this.session?.port.isOpen ?? false;
  }

  async open(path: string, baudRate: number): Promise<void> {
    if (!Number.isInteger(baudRate) || baudRate <= 0) {
      throw new Error("Set neonMdf.baudRate to a positive integer.");
    }
    if (this.session?.port.path === path && this.session.port.baudRate === baudRate && this.isOpen) {
      this.session.terminal.show();
      return;
    }
    await this.close();

    const output = new vscode.EventEmitter<string>();
    const port = new SerialPort({ path, baudRate, autoOpen: false });
    const decoder = new StringDecoder("utf8");
    let previousCR = false;
    let closing: Promise<void> | undefined;
    const disconnect = () => closing ??= new Promise<void>((resolve, reject) => {
      if (port.isOpen) {
        port.close((error) => error && port.isOpen ? reject(error) : resolve());
      } else {
        resolve();
      }
    }).catch((error: unknown) => {
      closing = undefined;
      throw error;
    });

    port.on("error", (error: Error) => output.fire(`\r\n[Serial error] ${error.message}\r\n`));
    port.on("close", () => output.fire("\r\n[Serial port closed]\r\n"));

    try {
      await new Promise<void>((resolve, reject) => {
        port.open((error) => error ? reject(error) : resolve());
      });
    } catch (error) {
      output.dispose();
      throw error;
    }

    const terminal = vscode.window.createTerminal({
      name: "NEON-MDF Serial Monitor",
      pty: {
        onDidWrite: output.event,
        open: () => {
          output.fire(`[Connected to ${path} at ${baudRate} baud, 8N1]\r\n`);
          port.on("data", (chunk: Buffer) => {
            const data = decoder.write(chunk);
            if (data.length === 0) {
              return;
            }
            // Preserve existing CRLF, including a pair split across serial reads.
            const normalized = ((previousCR ? "\r" : "") + data).replace(/(?<!\r)\n/g, "\r\n");
            output.fire(previousCR ? normalized.slice(1) : normalized);
            previousCR = data.endsWith("\r");
          });
        },
        handleInput: (data: string) => {
          if (port.isOpen) {
            port.write(data, (error) => {
              if (error) {
                output.fire(`\r\n[Serial write failed] ${error.message}\r\n`);
              }
            });
          }
        },
        close: () => {
          output.dispose();
          void disconnect().catch((error: unknown) => {
            vscode.window.showErrorMessage(`NEON-MDF: Could not release ${path}. ${String(error)}`);
          });
        },
      },
    });
    this.session = { port, terminal, disconnect };
    terminal.show();
  }

  async close(): Promise<void> {
    const session = this.session;
    if (session) {
      await session.disconnect();
      this.session = undefined;
      session.terminal.dispose();
    }
  }

  dispose(): void {
    void this.close().catch((error: unknown) => {
      vscode.window.showErrorMessage(`NEON-MDF: Could not release the serial port. ${String(error)}`);
    });
  }
}
