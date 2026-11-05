// List physical serial ports through the serialport native N-API binding.
// Listing is isolated from SYNC/READY probing so Node/Electron binding issues
// can be diagnosed without changing commands or UI. Tested with ST-Link VCP
// vendorId 0483, productId 3752, manufacturer STMicroelectronics.
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
