// debugArch.ts - tach phan DAC THU THEO KIEN TRUC CPU ra khoi debug.ts, de
// sau nay them target RISC-V (vd CH32V003) khong phai sua lai logic chung
// cua debug.ts - chi them 1 profile moi o day.
//
// Boi canh: debug.ts (Debug command) truoc day hardcode 2 thu chi dung
// duoc cho ARM Cortex-M:
//   1. `type: "cortex-debug"` - debug adapter cua VS Code, ARM-specific.
//   2. Lenh workaround reset khong dang tin cay (ghi DEMCR 0xE000EDFC +
//      AIRCR 0xE000ED0C qua preLaunchCommands/postLaunchCommands) - day la
//      thanh ghi System Control Space cua rieng ARMv7-M/ARMv8-M, KHONG ton
//      tai tren RISC-V (RISC-V co Debug Module rieng theo RISC-V Debug
//      Spec - dmcontrol/dmstatus... - hoan toan khac dia chi/ngu nghia).
// Phan con lai cua debug.ts (doc executable qua CMake File API, quy uoc
// duong dan openocd cfg/svd theo <target>, flash-truoc-roi-attach de tranh
// GDB `load` xoa mat image_header_t) KHONG phu thuoc kien truc - giu
// nguyen trong debug.ts, khong can tach.
//
// CHUA WIRING that voi RISC-V (vd CH32V003F4P6, xem trao doi voi user
// 2026-08-10) - ly do: toolchain/debug protocol cho chip do van con nhieu
// an so CHUA CO QUYET DINH (WCH dung 1 day rieng goi SDI/SWIO, KHONG PHAI
// SWD/JTAG chuan, can WCH-LinkE + 1 trong nhieu lua chon: OpenOCD ban fork
// rieng cua WCH/cong dong, `wlink`, `minichlink`, hay PicoRVD - moi cai co
// cach invoke/debug adapter khac nhau) - VA mcu-description hien CHUA co
// device RISC-V nao (chi co Cortex-M, xem schema arch: field da du kien
// san "riscv-rv32imac" nhung chua device nao dung). Wiring that can lam
// SAU khi co board that + quyet dinh toolchain, khong doan truoc o day -
// xem TODO trong resolveDebugArchProfile() ben duoi.

export interface DebugArchProfile {
  /** VS Code debug adapter type (vd "cortex-debug"). */
  debugType: string;
  /**
   * Lenh GDB monitor chay TRUOC launchCommands() cua debug adapter (xem
   * preLaunchCommands trong schema cua debug adapter tuong ung) - dung de
   * dam bao reset+halt dang tin cay TRUOC khi debug adapter tu lam viec
   * cua no (vd runToEntryPoint) - xem comment chi tiet trong cortexM
   * profile ben duoi cho ly do/cach verify that tren phan cung.
   */
  preLaunchCommands: string[];
  /** Lenh chay SAU launchCommands() - thuong de don dep trang thai bat o preLaunchCommands. */
  postLaunchCommands: string[];
}

// Cortex-M: profile DUY NHAT dang co that, verify tren phan cung WEACT_F412
// (xem memory/lich su session 2026-08-10) - KHONG doi logic o day, chi di
// chuyen nguyen xi tu debug.ts.
const CORTEX_M_PROFILE: DebugArchProfile = {
  debugType: "cortex-debug",
  // Bat DEMCR.VC_CORERESET (bit0 cua 0xE000EDFC, dac ta ARMv7-M) TRUOC khi
  // debug adapter tu lam reset - day la co che PHAN CUNG THAT cua chinh
  // CPU, khong phu thuoc polling/timing cua openocd: CPU se TU DONG vao
  // Debug state NGAY luc lay reset vector, TRUOC KHI chay lenh nao cua
  // Reset_Handler - bat ke reset duoc kich boi co che nao (SRST that hay
  // AIRCR phan mem). Can vi cortex-debug (servertype openocd) mac dinh
  // dung `monitor reset halt` - CHINH LA co che reset da xac nhan KHONG
  // DANG TIN CAY tren combo ST-Link clone + board WeAct nay (cung ly do vi
  // sao flash_jtag/flash_uart_stm32 dung AIRCR.SYSRESETREQ thay vi
  // `reset run` chuan, xem cmake/flash_targets.cmake). Da verify THAT: 4/4
  // lan lap lai voi dung lenh `reset halt` cua openocd (khong phai AIRCR
  // tu viet) cho ket qua GIONG HET nhau (SP = dinh RAM nguyen ven, chua he
  // dung stack - bang chung halt dung luc, truoc khi chay bat ky lenh nao).
  preLaunchCommands: ["monitor halt", "monitor mww 0xE000EDFC 0x00000001"],
  // Tat lai VC_CORERESET sau do (khong anh huong halt da xay ra - chi la
  // thanh ghi dieu khien cho LAN reset TIEP THEO) de khong lam anh huong
  // cac lan reset thu cong khac (vd flash_jtag/flash_uart_stm32 chay ben
  // ngoai VS Code) sau khi phien debug ket thuc.
  postLaunchCommands: ["monitor mww 0xE000EDFC 0x00000000"],
};

/**
 * Tra ve DebugArchProfile theo "arch family" (vd "cortex-m", tuong lai
 * "riscv"). CHUA co co che tu dong DO kien truc that cua target dang chon
 * (state.target) - device YAML (mcu-description/devices/<target>.yaml) da
 * co san field `arch:` (xem schema/common.schema.yaml dong 15, du kien san
 * "riscv-rv32imac" cho tuong lai) nhung CHUA duoc sinh ra thanh dang
 * machine-readable cho extension doc (hien chi Python/CMake doc truc tiep
 * YAML, extension khong co dependency parse YAML) - can lam THEM 1 buoc
 * (vd generate.py xuat kem 1 JSON manifest nho co field arch, hoac
 * extension tu doc YAML) KHI THAT SU co target RISC-V, khong lam truoc vi
 * chua biet hinh dang chinh xac can thiet (vd co can phan biet nhieu
 * "RISC-V family" khac nhau khong, hay 1 profile RISC-V la du).
 *
 * TAM THOI: luon tra ve profile Cortex-M (dung cho CA FU2 lan WEACT_F412
 * hien tai, ca 2 deu la Cortex-M) - KHONG doi hanh vi hien co. Goi voi bat
 * ky gia tri "family" nao khac "cortex-m" se throw ro rang thay vi am tham
 * dung nham profile sai, de loi bat ra ngay luc them target RISC-V dau
 * tien (khong bi bo sot buoc wiring).
 */
export function resolveDebugArchProfile(family: "cortex-m"): DebugArchProfile {
  if (family === "cortex-m") {
    return CORTEX_M_PROFILE;
  }
  throw new Error(
    `debugArch: chua co DebugArchProfile cho family "${family}" - them 1 profile moi vao debugArch.ts (xem TODO trong resolveDebugArchProfile()).`
  );
}
