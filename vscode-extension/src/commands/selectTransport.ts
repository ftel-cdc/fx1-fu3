// Choose transport with instructions matching the selected target.
import * as vscode from "vscode";
import { NeonMdfState, Transport } from "../state";

interface TransportItem extends vscode.QuickPickItem {
  value: Transport;
}

export async function selectTransportCommand(state: NeonMdfState): Promise<void> {
  const items: TransportItem[] = [
    {
      label: "$(plug) UART",
      description: state.target === "FU2"
        ? "ROMCODE USART update: select boot mode 100 and press RESET when the tool is ready"
        : "System Memory Bootloader STM32: target flash_uart_stm32, operation B0/PB2 by board",
      value: "uart",
    },
    {
      label: "$(circuit-board) JTAG/SWD",
      description: state.target === "FU2"
        ? "unsupported: configuration OpenOCD FU2 is a placeholder"
        : "flash via OpenOCD (target flash_jtag)",
      value: "jtag",
    },
  ];

  const picked = await vscode.window.showQuickPick(items, { placeHolder: "select transport" });
  if (!picked) {
    return;
  }
  await state.setTransport(picked.value);
}
