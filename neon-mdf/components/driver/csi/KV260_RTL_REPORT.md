# FTEL MIPI CSI-2 RX on hardware: KV260 bring-up report (RTL)

Date 2026-11-27. This report covers RTL behaviour seen while bringing up the receiver on an AMD Kria
KV260 with a Sony IMX219, including timing closure and FPGA integration. The driver findings are in
[KV260_DRIVER_REPORT.md](KV260_DRIVER_REPORT.md). The setup is the same: 2 lanes, 800 Mb/s per lane,
RAW10 640x480.

The RTL was FX1 `hw/rtl/ip/fox_mipi_csi2_ctl` dev 5376e2e. Its `dphy/` and `rx_controller/` files are
byte-identical to `fu3_dphy` 5d3a691 and `MIPI_CSI2_RX_Controller` a653ee8. The FX1 superproject pins
this submodule at 18fb3a8 (`init`, `.gitkeep` only), so a recursive checkout of FX1 gets no RTL.

**Summary.** On the board the receiver is functional:
- colour bars are pixel-exact on both the AXI4-Stream and the Block 13 DMA paths;
- 301 frames in 10 s with 0 ECC, CRC, drop or lane-abort errors;
- the e2e XSIM bench (AMD CSI-2 TX to FU3 D-PHY to the controller) passes 6/6 scenarios at
  400 and 912 Mb/s.

The open points are R1 (Block 13 timing) and R5 (the FPGA integration top). The others are
clarifications.

## R1: Block 13 per-beat line-address multiply limits `sys_clk`

`m_fpt_dma.sv` (pixel writer, about lines 545-560) computes, in the same cycle that decides the raw
FIFO write, all of the following:
- `w_line_addr = buf_addr + vc_offset + line_idx * stride`, a 16 x 17 multiply;
- `w_beat_addr` and the clip compare against `bytes_per_line` and the region end;
- `w_pw_err`, `w_write`, the burst close and the FIFO push.

On the xck26-2 this path is about 10.5 ns: 26-29 logic levels, using a DSP and 8 CARRY8. The
worst endpoint runs from the B08 `reg_dma_fifo_rd_ptr` to the B13 `u_raw_data_fifo` / `u_raw_cmd_fifo`
write enables. At 150 MHz the WNS was -3.51 ns over 807 endpoints, so the KV260 build runs `sys_clk`
and `mm_aclk` at 93.75 MHz.

The default `PARA_TS_INCR` (4 ns) suggests a 250 MHz `sys_clk` on silicon. This path should be
checked in the ASIC synthesis.

Suggestion: keep the line address incrementally. Load it with `buf_addr + vc_offset` at SOF and add
`stride` at EOL. This removes the multiply from the per-beat path.

## R2: abort beats on `axis_enable` 1 to 0 when no frame is open

When `STREAM_CONFIG.axis_enable` falls, B10 `w_stream_stop` raises `asm_abort_req` with mask 2'b11.
B12's Finalize Sequencer then sends one abort beat per TID: TLAST, TKEEP = 0, TUSER = 2'b10,
TDATA = 0. It does so even when no frame is open on either TID. The e2e bench sees two such beats
during configuration, when the driver clears `axis_enable` before changing `stream_format`.

Please confirm this is intended. Consumers such as the ISP adapter must treat a TKEEP = 0 / TUSER[1]
beat as a frame close, not as a line.

## R3: non-continuous clock (outside MAS §6.3.6), as observed

The IMX219 only has non-continuous clock. With it the receiver works with 0 errors. Because
`rxbyteclkhs` stops at the end of each burst, the tail of a burst (the last line or the Frame End) is
delivered only when the next burst starts. A capture therefore completes at the next Frame Start. No
data is lost, and frames and lines match.

This is reported for information. The MAS already excludes the mode, and the FX1 PHY backend refuses it.

## R4: `PARA_PHY_RST_CYCLES` is counted in `lite_aclk`

The MAS requires `phy_core_rst` for at least 40 `core_clk` (200 ns at 200 MHz). The PHY Reset
Sequencer counts `PARA_PHY_RST_CYCLES` = 40 in `lite_aclk`, so the requirement holds only while
`lite_aclk` <= `core_clk`. On the KV260 the pulse is 427 ns (93.75 MHz). On a system with a faster
`lite_aclk` the parameter has to be raised.

Suggestion: document the dependency, or derive the count from both clock frequencies.

The driver's wait on `CORE_STATUS.reset_in_progress` was measured as effective: the first read after
the write saw the bit set 1000/1000 times.

## R5: FPGA integration top `m_fpt_camera_vp1902_top`

The top was built for the KV260 as delivered. The only change was a copy of `m_fpt_vp1902_dphy_io`
with `SIM_DEVICE("ULTRASCALE_PLUS")` and the termination left to the XDC (MIPI_DPHY_DCI). Timing was
met, and on the board the results matched the KV260 front end: pixel-exact, 0 errors in 10 s,
neon_csi 300/300 SW_VALID. Three points, all specific to the FPGA:

1. **No IDELAY.** The HS sampling point is set by the clock path (IBUFDS_DPHY, BUFG, clock tree)
   against the data route, so it changes with every place and route. The KV260 front end has an
   IDELAYE3 per data lane that software sweeps (512 taps, about 1 UI at 800 Mb/s). The error-free
   window was taps 80..376 in one build and 0..224 in another. With no delay, the first build would
   have failed at tap 0, and software could not have corrected it. Suggestion: a controllable input
   delay in the IO wrapper (on Versal, the XPHY delay), plus a sweep at bring-up.
2. **RTL clock gate.** Without `FPT_SIGNOFF` the byte-clock gate is the RTL latch and AND (1 LDCE),
   so the byte clock goes through fabric logic. The path from the reset synchroniser to the gate is
   timed with latch time borrowing (reported slack 0.000 ns). The KV260 front end maps
   `m_fpt_lib_cg` to BUFGCE (CE_TYPE SYNC) and `m_fpt_lib_clk_inv` to an inverter under
   `FPT_SIGNOFF`, which keeps the byte clock on dedicated clock resources.
3. **Placement of the byte-clock gate.** The path from the reset synchroniser (HS clock) to the
   BUFGCE CE has a 2.5 ns budget. Left to the placer, it landed three clock regions away from the
   MIPI bank: 3.7 ns of route, -0.406 ns slack. A pblock that keeps `u_afe_adapter_0` in the clock
   region of the camera pins fixed it, to +0.681 ns.

Constraints used on the KV260, for reference:
- the IBUFDS_DPHY `HSRX_DISABLE` to `HSRX_O` arc disabled (otherwise the core clock propagates onto
  the HS clock);
- the byte clock declared as a generated clock (/4) of the HS clock;
- multicycle 3/3 from `reg_half_*` to `reg_hs_pre`;
- `set_max_delay -datapath_only` 2 ns from the data pins to the samplers;
- asynchronous clock groups.

## R6: `sys_clk` below `rxbyteclkhs`

At 93.75 MHz (see R1) `sys_clk` is below the 100 MHz byte clock, which the controller documentation
requires to be at least equal. The KV260 build compensates with `PARA_ASYNC_FIFO_DEPTH` = 256. A
640-pixel line builds about 25 entries of backlog and drains during line blanking. This holds only
for short lines. It is reported as an FPGA constraint, not an RTL defect.
