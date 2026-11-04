// openMonitor.ts - open Serial Monitor native (see serialMonitor/monitor.ts) on the selected port in
// NeonMdfState.
import { NeonMdfState } from "../state";
import { NeonMdfSerialMonitor } from "../serialMonitor/monitor";

export async function openMonitorCommand(state: NeonMdfState, monitor: NeonMdfSerialMonitor): Promise<void> {
  await monitor.open(state);
}
