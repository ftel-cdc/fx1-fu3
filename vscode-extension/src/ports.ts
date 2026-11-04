// ports.ts - liet ke cong serial THAT qua serialport (native N-API
// binding). Da xac nhan binding nay load duoc duoi Node thuong (thay ST-Link
// that: vendorId 0483, productId 3752, manufacturer STMicroelectronics) -
// buoc nay xac nhan lai duoi chinh Extension Development Host (Electron
// ABI). Tach thanh module rieng, chi lam 1 viec (liet ke cong) - KHONG lam
// giao thuc SYNC/READY (xem serialMonitor/portProbe.ts o buoc sau), de neu
// serialport that bai duoi Electron thi chi phai thay module nay (vd goi
// qua child_process Node he thong) ma khong dung toi commands/UI da viet.
import { SerialPort } from "serialport";

export interface PortInfo {
  path: string;
  manufacturer?: string;
  vendorId?: string;
  productId?: string;
}

export async function listPorts(): Promise<PortInfo[]> {
  const ports = await SerialPort.list();
  return ports.map((p) => ({
    path: p.path,
    manufacturer: p.manufacturer,
    vendorId: p.vendorId,
    productId: p.productId,
  }));
}
