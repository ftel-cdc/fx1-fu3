# neon_csi on hardware: KV260 bring-up report (driver)

Date 2026-11-27. This report covers `csi/platform/fx1/neon_csi.c` and the PHY layer at
`feature/drivers_fx1` 1739bde. They were run unmodified, except for one patch noted below, against the
FTEL MIPI CSI-2 RX controller RTL on an AMD Kria KV260 with a Sony IMX219 sensor. The reference is
`FTEL_IP_MIPI_CSI-2_CONTROLLER_MAS_v1.4` (MAS) and its register list. The RTL findings are in
[KV260_RTL_REPORT.md](KV260_RTL_REPORT.md).

**Summary.** The driver works on hardware once two deviations from MAS §9.1 are fixed (D1, D2).
After the fixes, `configure`, `start`, `acquire_frame`, `release_frame` and `stop` all run. Every
buffer is SW_VALID, and the pixels match a golden colour-bar frame bit for bit. Both fixes are in
the driver only; neither the RTL nor the MAS needs a change.

## Test setup

| Item | Value |
|---|---|
| Board | Kria KV260 (xck26), FPGA prototype of the receiver |
| CPU | MicroBlaze V **rv32i** (FX1 is rv32imafc), bare-metal, 93.75 MHz, data cache on DDR |
| RTL | FX1 `hw/rtl/ip/fox_mipi_csi2_ctl` dev 5376e2e: FU3 D-PHY digital + CSI-2 controller B01-B14 (identical to `fu3_dphy` 5d3a691 and `MIPI_CSI2_RX_Controller` a653ee8). Both the KV260 front end and the team top `m_fpt_camera_vp1902_top` were tested. |
| Clocks | D-PHY core 187.5 MHz; `sys_clk` = `mm_aclk` = `lite_aclk` = 93.75 MHz |
| Sensor | IMX219, 2 lanes, 800 Mb/s per lane, RAW10 640x480, 30 fps, colour bars and live image. **Non-continuous clock only**, see D3. |
| Port | `fx1_fw.h`: MMIO, fences, `fx1_dma_*` mapped to cache flush and invalidate. `fx1_registers.h`: the FX1 map with only the CSI-2 base moved. A `csi-dphy0` PHY backend for the FPGA front end. The driver sources are compiled in place. |
| Config | `lane_count` 2, `lane_rate_bps` 800e6, RAW10 to `NEON_CSI_PACKING_UNPACKED16`, 2 DMA buffers of 1280 x 480 |

## Results

| Step | Unmodified driver | With D1 + D2 fixed |
|---|---|---|
| `neon_phy_get`, `neon_csi_init`, `get_caps`, `calculate_layout` | OK | OK |
| `neon_csi_configure` | **NEON_TIMEOUT** (D1) | OK |
| `neon_csi_start` + sensor stream | not reached | OK. `active_lanes_applied` matches once the sensor streams. |
| `acquire_frame` / `release_frame`, 10 s | not reached; with D1 alone fixed, **every buffer SW_ERROR** (D2) | 300/300 SW_VALID, `frame_id` consecutive, timestamps 33.3 ms apart |
| Pixel check | n/a | Both DMA buffers identical to the golden frame (307200/307200 px) |
| AXI4-Stream path with no consumer (ISP absent) | n/a | DMA unaffected: 150/150 SW_VALID |
| `neon_csi_stop` | n/a | OK |

## D1 (must fix): `active_lanes_applied` is polled at step 4; MAS confirms it at step 13

`csi_link_up()` writes `PROTOCOL_CONFIG.active_lanes`, then polls `active_lanes_applied` with
`CSI_LANES_APPLIED_POLLS` before it enables any lane (`neon_csi.c` around line 1200). MAS §9.1 step 4
reads: "Lane Management adopts active_lanes only once rxbyteclkhs is running; the value is confirmed at
step 13". Step 13 runs after the sensor has started streaming: poll `PHY_STATUS.cl_rxclkactivehs`,
then compare `active_lanes_applied` with `active_lanes`. §6.4 repeats this rule.

Why it cannot complete at step 4:
- Lane Management latches `active_lanes` on `rxbyteclkhs` (`m_fpt_lane_management.sv`).
- The D-PHY runs `rxbyteclkhs` only with the clock lane enabled and in HS (`m_fpt_dphy_rx_cl_ctrl.sv`:
  `byteclk_en = (state == CL_HS_CLK)`).
- At step 4 the lanes were just disabled by the `dphy_rst_req` of step 2, and the sensor does not stream yet.

Measured on the board: `PROTOCOL_CONFIG` = 0x301, meaning `active_lanes` = 2 lanes and `applied` = 0,
and the byte clock does not toggle. Streaming the sensor during `configure()` does not help either,
because the clock lane is still disabled.

The VP probably does not show this because it does not model the gated byte clock.

Fix tested on hardware: drop the poll from `csi_link_up()`. The tested variant also moved the
write after step 6; keeping it at step 4 as in the MAS is equivalent. Confirm the value after the
sensor starts:

```diff
@@ csi_link_up()
-    for (polls = 0U; CSI_GET(PROTOCOL_CONFIG, ACTIVE_LANES_APPLIED, fx1_read32(CSI_REG(PROTOCOL_CONFIG))) != active_lanes;
-         polls++) {
-        if (polls >= CSI_LANES_APPLIED_POLLS) {
-            return NEON_TIMEOUT;
-        }
-    }
```

Step 13 needs a place in the API, because `start()` returns before the application starts the sensor.
Two options:
- a new call, for example `neon_csi_confirm_link(handle, timeout_us)`, made by the application or the
  camera runtime after the sensor stream-on;
- a check on the first `acquire_frame()` that reports a mismatch as an error.

In both cases the timeout is wall-clock time to the sensor's first HS burst (milliseconds), not
`CSI_LANES_APPLIED_POLLS` register reads. On the board `applied` was correct on every run, because
the clock lane goes HS before the first data burst. The check is a safety net, not a wait.

## D2 (must fix): `META_AREA_SIZE` = 0 with metadata off; every buffer completes SW_ERROR

`calculate_layout()` (line 393) and `csi_write_dma_path()` (line 1367) use a metadata area of 0 when
`metadata.enable` is false. MAS §9.1 step 9 requires `META_AREA_SIZE` to be at least 0x80 plus the
largest embedded payload, with one buffer occupying `2 x (DMA_BUFFER_SIZE + META_AREA_SIZE)`.

Block 13 writes the frame record (`CSI_RECORD_BYTES`) for every capture, whether or not metadata
capture is enabled. When the record does not fit, the metadata writer aborts the buffer
(`m_fpt_dma.sv`, `w_wr_err_mw`).

Seen on the board with metadata off:
- every buffer completes `NEON_CSI_BUFFER_SW_ERROR`;
- `last_valid_line` is 479 and the pixels are intact;
- `BUFFER_STATUS.dma_overflow` and `IRQ_STATUS.buffer_overflow` / `buf_done_err` are set every frame.

Fix: always reserve the record area, as in this sketch, and count it in `layout.buffer_bytes`:

```c
meta = config->metadata.enable ? config->metadata.area_size_bytes : CSI_META_PAYLOAD_OFFSET; /* 0x80 */
```

Measured:

| Metadata | `META_AREA_SIZE` | Result |
|---|---|---|
| off | 0 (current driver) | every buffer SW_ERROR |
| off | 0x100 (written after `configure`) | 150/150 SW_VALID |
| on | 4 KiB | 300/300 SW_VALID |

0x80 itself was not measured; it is the MAS minimum.

## D3 (information): non-continuous clock

The FX1 PHY backend rejects `continuous_clock = false`. This matches MAS §6.3.6: non-continuous clock
behaviour is not supported in this release. The camera HALs (IMX675, OS04C10) must configure the
sensor for continuous clock.

The IMX219 has no continuous mode, so the KV260 PHY backend accepts it. On the board it still works
with 0 errors. The tail of each burst is delivered with the next burst, so a buffer completes at the
next Frame Start, one frame-blanking time later.

## D4 (information): D-PHY reset wait (steps 2-3) is effective

After `dphy_rst_req` the driver polls `CORE_STATUS.reset_in_progress`. A write and a read travel on
separate AXI channels, so the first read could in principle overtake the write and see 0. This was
measured on the board: 1000 write-then-read pairs, with and without a fence. The first read saw
`reset_in_progress` = 1 every time, and the second read saw 0. No change is needed.

## D5 (review): cache maintenance of the pixel regions

`acquire_frame()` calls `fx1_dma_complete()` only on the metadata records. Pixel regions are left to
the caller, which the comment states. `release_frame()` does not call `fx1_dma_prepare()` on the
buffer it hands back.

With a write-back cache that is not coherent with the DMA, which is the case on the KV260, a dirty
line left in a buffer can be evicted over data the DMA writes later. That happens if the application
wrote into the buffer, for example while processing in place. This was not observed in the tests,
which only read the buffers.

Suggestion: call `fx1_dma_prepare(FROM_DEVICE)` on both pixel regions in `start()` and in
`release_frame()`. Alternatively, document this as a caller duty next to the existing note on
`acquire_frame()`.

## Not covered by this test

- Interrupt-driven operation: the test polls, because the KV260 build has no interrupt controller.
- FreeRTOS.
- rv32imafc code generation.
- The TPG path, two virtual channels, embedded-data extraction, `neon_csi_recover()`.
- Line rates other than 800 Mb/s.
