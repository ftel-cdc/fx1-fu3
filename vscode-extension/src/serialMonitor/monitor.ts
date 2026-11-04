// monitor.ts - Serial Monitor native (khong shell ra `python3 -m
// serial.tools.miniterm` nua) - mo cong bang serialport (da proven o
// ports.ts), doc byte stream theo dong, ghi ra 1 OutputChannel dung chung.
// Chi 1 phien mo tai 1 thoi diem - open() luon dong phien cu truoc, tranh 2
// tien trinh/handle cung giu 1 cong (dung loi "Could not exclusively lock
// port" da gap truoc day khi mo miniterm 2 lan).
import * as vscode from "vscode";
import { SerialPort } from "serialport";
import { NeonMdfState } from "../state";

export class NeonMdfSerialMonitor implements vscode.Disposable {
  private readonly output: vscode.OutputChannel;
  private port: SerialPort | undefined;
  private opening: Promise<void> | undefined;
  private lineBuf = "";

  constructor() {
    this.output = vscode.window.createOutputChannel("NEON-MDF Serial Monitor");
  }

  async open(state: NeonMdfState): Promise<void> {
    if (!state.port) {
      vscode.window.showWarningMessage(
        'NEON-MDF: chua chon cong serial - bam vao "chon cong" tren status bar truoc.'
      );
      return;
    }

    await this.close();
    this.lineBuf = "";
    this.output.show(true);
    this.output.appendLine(`--- Mo ${state.port} @ ${state.baudRate} 8N1 ---`);

    const sp = new SerialPort({ path: state.port, baudRate: state.baudRate, autoOpen: false });
    this.port = sp;

    sp.on("data", (chunk: Buffer) => this.handleData(chunk));
    sp.on("error", (err: Error) => this.output.appendLine(`--- Loi serial: ${err.message} ---`));
    sp.on("close", () => this.output.appendLine("--- Da dong cong ---"));

    this.opening = new Promise<void>((resolve) => {
      sp.open((err) => {
        if (err) {
          this.output.appendLine(`--- Khong mo duoc cong: ${err.message} ---`);
          if (this.port === sp) {
            this.port = undefined;
          }
        }
        resolve();
      });
    });
    await this.opening;
  }

  private handleData(chunk: Buffer): void {
    this.lineBuf += chunk.toString("utf8");
    const lines = this.lineBuf.split("\n");
    this.lineBuf = lines.pop() ?? "";
    for (const line of lines) {
      this.output.appendLine(line.replace(/\r$/, ""));
    }
  }

  async close(): Promise<void> {
    const port = this.port;
    const opening = this.opening;
    this.port = undefined;
    this.opening = undefined;
    if (!port) {
      return;
    }
    await opening;
    await new Promise<void>((resolve) => {
      if (port.isOpen) {
        port.close(() => resolve());
      } else {
        resolve();
      }
    });
  }

  dispose(): void {
    void this.close();
    this.output.dispose();
  }
}
