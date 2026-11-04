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
        ? "ROMCODE USART update: chon boot mode 100, bam RESET khi tool san sang"
        : "System Memory Bootloader STM32: target flash_uart_stm32, thao tac B0/PB2 theo board",
      value: "uart",
    },
    {
      label: "$(circuit-board) JTAG/SWD",
      description: state.target === "FU2"
        ? "Chua ho tro: cau hinh OpenOCD FU2 dang la placeholder"
        : "Nap qua OpenOCD (target flash_jtag)",
      value: "jtag",
    },
  ];

  const picked = await vscode.window.showQuickPick(items, { placeHolder: "Chon transport" });
  if (!picked) {
    return;
  }
  await state.setTransport(picked.value);
}
