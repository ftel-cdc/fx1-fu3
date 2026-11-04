// portProbe.ts - tu dong do cong serial dang chay bootloader TU VIET cua
// SDK, mirror giao thuc SYNC/READY ma components/bootloader/neon_bootloader_main.c
// noi TREN CHIP (SYNC_BYTE, READY_BYTE, SYNC_TIMEOUT_MS/SYNC_RETRY_S - xem
// chu thich trong file do). KHONG re-implement toan bo giao thuc
// header/CRC/chunk o day - chi la 1 tien ich "tim cong tay" doc lap (xem
// commands/detectPort.ts), KHONG con target CMake flash nao goi qua UART
// protocol nay nua (khong flash_uart/flash_bootloader/flash_all - da xoa,
// script PC-side cua protocol nay - tools/neon_flash_app_firmware.py - cung
// da xoa theo, xem cmake/flash_targets.cmake, gio chi con flash_jtag/
// flash_uart_stm32, ca 2 deu nap TRON BO [bootloader+header+app], khong dung
// giao thuc nay). Bootloader tren chip VAN CON chay protocol nay moi lan
// boot (chi la khong con cong cu PC-side nao chu dong noi chuyen voi no qua
// UART nua) nen probe nay van dung duoc.
//
// Bootloader chi lang nghe SYNC trong 1 CUA SO NGAN DUY NHAT ngay sau reset
// (xem SYNC_TIMEOUT_MS trong components/bootloader/neon_bootloader_main.c) -
// cua so nay duoc CHIA SE cho MOI cong ung vien, nen phai probe SONG SONG
// (mo tat ca cong cung luc, gui SYNC deu tren tung cong) - probe TUAN TU se
// het gio truoc khi thu het danh sach cong.
import { SerialPort } from "serialport";
import { listPorts } from "../ports";

// Phai khop CHINH XAC voi components/bootloader/neon_bootloader_main.c - doi 1 ben thi doi ca 2.
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
        // Fire-and-forget: mot so cong AO cua he dieu hanh (vd Bluetooth
        // Incoming Port, debug-console tren macOS) co the TREO VO HAN o
        // callback cua close() - da xac nhan bang test that (Promise.all()
        // ca cum khong bao gio resolve du tat ca finish() da chay). Da co
        // ket qua (found) roi thi khong can doi close() xong moi resolve.
        sp.close(() => {});
      }
      resolve(found);
    };

    // Dat timeout NGAY TU DAU (truoc ca khi mo cong), khong phai sau khi
    // mo thanh cong - mot so "cong ao" cua he dieu hanh (vd
    // /dev/tty.debug-console tren macOS) co the treo vo han o buoc open(),
    // khong bao gio goi callback loi hay thanh cong. Neu timeout chi bat
    // dau sau khi mo xong, 1 cong treo se lam ca Promise.all() cua
    // detectBootloaderPort() treo vinh vien theo (da xac nhan bang test
    // that - sua truoc khi bug nay lot vao production).
    const timeout = setTimeout(() => finish(false), SYNC_RETRY_MS);

    sp.on("error", () => finish(false));

    sp.open((err) => {
      if (err) {
        finish(false);
        return;
      }
      // XA BO du lieu con ton trong buffer nhan CUA HE DIEU HANH truoc khi
      // gan listener "data" (tuong duong ser.reset_input_buffer() cua
      // pyserial). Thieu buoc nay da xac nhan bang test that: byte
      // "rac" con lai tu app CU (dang chay TRUOC khi board duoc reset) van
      // con nam trong buffer nhan cua driver luc mo cong, khien probe ket
      // luan SAI la "khong phai bootloader" chi sau ~30ms, du bootloader
      // that su van dang cho SYNC dung luc do.
      sp.flush(() => {
        if (settled) {
          return;
        }
        sp.on("data", (chunk: Buffer) => {
          // Nhan BAT KY byte nao la du de ket luan - KHONG gui them SYNC sau
          // khi da co phan hoi (tranh lam lech giao thuc phia bootloader
          // thuc su neu day dung la no - bootloader khong co co che resync).
          finish(chunk.length > 0 && chunk[0] === READY_BYTE);
        });
        interval = setInterval(() => sp.write(Buffer.from([SYNC_BYTE])), SYNC_POLL_MS);
      });
    });
  });
}

/**
 * RESET board TRUOC khi goi ham nay (bootloader chi cho SYNC trong 1 cua so
 * ngan sau reset). Tra ve duong dan cong dau tien tra loi dung giao thuc,
 * undefined neu khong cong nao phan hoi trong cua so cho.
 */
export async function detectBootloaderPort(): Promise<string | undefined> {
  const allCandidates = await listPorts();
  // Chi probe cong co vendorId (thiet bi USB-serial that - ST-Link VCP,
  // CH340, FTDI...) - cac "cong ao" cua macOS (Bluetooth-Incoming-Port,
  // debug-console, tai nghe Bluetooth...) khong bao gio co vendorId VA da
  // xac nhan bang test that: mo/dong cac cong nay co the treo VINH VIEN o
  // tang native (khong lien quan gi den logic JS o day, timeout/resolve
  // van chay dung) - khong bao gio la board that nen khong co ly do probe.
  const candidates = allCandidates.filter((c) => c.vendorId !== undefined);
  if (candidates.length === 0) {
    return undefined;
  }

  const results = await Promise.all(candidates.map((c) => probeOnePort(c.path)));
  const foundIndex = results.findIndex((ok) => ok);
  return foundIndex >= 0 ? candidates[foundIndex].path : undefined;
}
