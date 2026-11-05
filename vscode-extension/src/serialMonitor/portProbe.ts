// Probe the SDK bootloader SYNC/READY protocol, separately from FU2 ROMCODE.
// Keep constants synchronized with neon_bootloader_main.c. The bootloader
// listens only briefly after reset, so probe candidate ports in parallel.
// Full-image flash_jtag/flash_uart_stm32 do not use this UART protocol.
import { SerialPort } from "serialport";
import { listPorts } from "../ports";

// Must match components/bootloader/neon_bootloader_main.c exactly; update both sides together.
const SYNC_BYTE = 0x55;
const READY_BYTE = 0x52;
const SYNC_RETRY_MS = 9500;
const SYNC_POLL_MS = 100;
const PROBE_BAUD_RATE = 115200;

function probeOnePort(path: string): Promise<boolean> {
  return new Promise((resolve) => {
    let settled = false;
    let interval: ReturnType<typeof setInterval> | undefined;

    const sp = new SerialPort({ path, baudRate: PROBE_BAUD_RATE, autoOpen: false });

    const finish = (found: boolean) => {
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
        // Fire-and-forget: some AO ports of the operating he (e.g. Bluetooth Incoming Port, macOS
        // debug-console) can TREO infinitely at callbacks of close() - confirmed by real test
        // (Promise.all() even if no resolve time is announced enough to disable all running
        // finish()). once found, no need to change close() after each resolve.
        sp.close(() => {});
      }
      resolve(found);
    };

    // set timeout NOW from the beginning (before opening port), not after successful opening - some
    // "ports" of the operating he (e.g. /dev/tty.debug-console on macOS) can hang indefinitely at
    // step open(), do not tell the time to callback error please succeed. if the timeout starts
    // only after opening is finished, a hanging port will make both Promise.all() of
    // detectBootloaderPort() hang permanently according to (confirmed by real test - fix before
    // this bug lots and production).
    const timeout = setTimeout(() => finish(false), SYNC_RETRY_MS);

    sp.on("error", () => finish(false));

    sp.open((err) => {
      if (err) {
        finish(false);
        return;
      }
      // XA sets enough children in the receive buffer of HE operator before assigning listener "data"
      // (equivalent to path ser.reset_input_buffer() of pyserial). missing this step has been
      // confirmed by real test: byte "rac" child from old app (running before board reset) still
      // male children in driver's receive buffer when open port, whenen probe concluded that SAI
      // is "not bootloader" only after ~30ms, enough actual bootloader is still used by SYNC at
      // that time.
      sp.flush(() => {
        if (settled) {
          return;
        }
        sp.on("data", (chunk: Buffer) => {
          // Any received byte is enough to finish probing. Stop sending SYNC after a response to avoid disrupting a bootloader with no resynchronization support.
          // included (the real-time bootloader interface is used if the use date is no - the
          // bootloader does not have resync cover).
          finish(chunk.length > 0 && chunk[0] === READY_BYTE);
        });
        interval = setInterval(() => sp.write(Buffer.from([SYNC_BYTE])), SYNC_POLL_MS);
      });
    });
  });
}

/**
 * RESET board before calling this function (bootloader only for SYNC in 1 of the short numbers after
 * reset). look up the first curved path, check for authentication errors, undefined if not the
 * inner part of the given number.
 */
export async function detectBootloaderPort(): Promise<string | undefined> {
  const allCandidates = await listPorts();
  // Only probe ports with vendorId, representing physical USB serial devices such as ST-Link VCP, CH340 and FTDI -
  // macOS "ponds" (Bluetooth-Incoming-Port, debug-console, at listening to Bluetooth...) do not
  // never have vendorId. Hardware testing confirmed that opening or closing these virtual ports can hang indefinitely in
  // the native layer. The JavaScript timeout/resolve logic still runs, but these ports are never physical boards
  // board so there is no reason for that probe.
  const candidates = allCandidates.filter((c) => c.vendorId !== undefined);
  if (candidates.length === 0) {
    return undefined;
  }

  const results = await Promise.all(candidates.map((c) => probeOnePort(c.path)));
  const foundIndex = results.findIndex((ok) => ok);
  return foundIndex >= 0 ? candidates[foundIndex].path : undefined;
}
