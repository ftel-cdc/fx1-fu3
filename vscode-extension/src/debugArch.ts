// CPU-specific debugger profiles, separated from the generic debug.ts flow.
// The Cortex-M profile was verified on WEACT_F412. RISC-V needs its own
// Debug Module commands and verified tools; it must not use ARM DEMCR writes.
export interface DebugArchProfile {
  /** VS Code debug adapter type (vd "cortex-debug"). */
  debugType: string;
  /**
   * GDB monitor commands run before launchCommands() of the debug adapter (see preLaunchCommands in
   * schema of compatible debug adapters) - used to make sure reset+halt is running correctly before
   * the debug adapter before operations such as runToEntryPoint. See the Cortex-M profile below for the
   * rationale and hardware verification.
   */
  preLaunchCommands: string[];
  /** command runs after launchCommands() - usually to don dep status enabled in preLaunchCommands. */
  postLaunchCommands: string[];
}

// Cortex-M: the only implemented profile, verified on WEACT_F412 hardware; see the session notes
// 2026-08-10). Preserve the implementation and move it
// moved unchanged from debug.ts.
const CORTEX_M_PROFILE: DebugArchProfile = {
  debugType: "cortex-debug",
  // Enable DEMCR.VC_CORERESET (bit0 of 0xE000EDFC, ARMv7-M) before the debug adapter resets the target.
  // This CPU hardware mechanism does not depend on OpenOCD polling or timing:
  // The CPU will automatically enter the Debug state NOW to get the reset vector, before running
  // instruction in Reset_Handler, regardless of whether reset uses physical SRST
  // let AIRCR software). need to use default cortex-debug (servertype openocd) using `monitor reset
  // halt` - this is a confirmed not trusting reset cover on this ST-Link clone + WeAct board combo
  // (same reason why flash_jtag/flash_uart_stm32 uses AIRCR.SYSRESETREQ see because `reset run`
  // standard behavior; see cmake/flash_targets.cmake). Verified in four of four OpenOCD `reset
  // halt` command, rather than an AIRCR write, gave identical results: SP at the top of RAM, before stack
  // a general halt during execution, before any instruction runs).
  preLaunchCommands: ["monitor halt", "monitor mww 0xE000EDFC 0x00000001"],
  // re-disable VC_CORERESET afterwards (not the halt-directed image that was milled out - just the
  // control register for the next reset) so as not to direct the image to other curved message
  // resets outside VS Code, such as flash_jtag/flash_uart_stm32, after the debug session
  // authentication.
  postLaunchCommands: ["monitor mww 0xE000EDFC 0x00000000"],
};

/**
 * look up DebugArchProfile by "arch family" (e.g. "cortex-m", future "riscv"). no automatic masking
 * exists for the target being selected
 * state.target, from mcu-description/devices/<target>.yaml, already
 * has field `arch:` (see dynamic schema/common.schema.yaml 15, enough "riscv-rv32imac" for the future)
 * but has not been generated as machine-readable for extension read (currently only Python/CMake
 * reads YAML directly, extension does not have dependency parse YAML) - requires one more step
 * (e.g. generate.py outputs a small JSON manifest with field arch, or extension from read YAML)
 * when a RISC-V target is available; the exact setup must be
 * defined (e.g. whether to know many different "RISC-V families", 1 RISC-V profile is sufficient).
 *
 * Temporary: check the Cortex-M profile (used for both FU2 and current WEACT_F412, both are Cortex-M)
 * - do not change the existing behavior. call with the "family" value turned on periodically other
 * than "cortex-m" raises an explicit error instead of silently selecting an incompatible profile, so failures
 * pop out immediately when adding target RISC-V first (no wiring step leftover).
 */
export function resolveDebugArchProfile(family: "cortex-m"): DebugArchProfile {
  if (family === "cortex-m") {
    return CORTEX_M_PROFILE;
  }
  throw new Error(
    `debugArch: no DebugArchProfile for family "${family}" yet - add 1 profile per debugArch.ts (see TODO in resolveDebugArchProfile()).`
  );
}
