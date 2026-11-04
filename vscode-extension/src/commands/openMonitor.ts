// openMonitor.ts - mo Serial Monitor native (xem serialMonitor/monitor.ts)
// tren cong dang chon trong NeonMdfState.
import { NeonMdfState } from "../state";
import { NeonMdfSerialMonitor } from "../serialMonitor/monitor";

export async function openMonitorCommand(state: NeonMdfState, monitor: NeonMdfSerialMonitor): Promise<void> {
  await monitor.open(state);
}
