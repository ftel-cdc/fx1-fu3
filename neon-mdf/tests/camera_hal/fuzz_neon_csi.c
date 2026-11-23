/* Model-based test of the production CSI register driver. No metadata: the fixture does not DMA DDR.
 * Checks oldest acquisition across the 14-bit wrap, no duplicate leases, out-of-order release,
 * recovery and restart. MMIO timing and metadata capture need VP/hardware coverage. */
#include <stdio.h>
#include <stdlib.h>
#include "host/csi_mmio.h"
#include "csi/platform/fx1/neon_csi_fx1.h"
#include "mipi/platform/fx1/fx1_mipi_simulation.h"

static uint64_t rng;
static unsigned rnd(unsigned n)
{
    rng = rng * UINT64_C(6364136223846793005) + UINT64_C(1442695040888963407);
    return (unsigned)((rng >> 33) % n);
}
#define REQUIRE(condition) do { if (!(condition)) { \
    fprintf(stderr, "CSI model failure: seed=%u step=%u: %s\n", seed, step, #condition); return 1; \
} } while (0)

static int run(unsigned seed)
{
    unsigned step = 0U, next_id = 0x3ffcU, serial = 1U;
    unsigned age[2] = {0U, 0U};
    int ready[2] = {0, 0}, held[2] = {0, 0};
    neon_mipi_handle_t link;
    neon_csi_handle_t csi;
    neon_csi_config_t config;
    neon_mipi_config_t mc = {NEON_MIPI_DIRECTION_RX, 4U, 1000000000UL, true};
    rng = seed;
    csi_mmio_reset();
    REQUIRE(neon_mipi_init(&link) == NEON_OK);
    REQUIRE(neon_mipi_acquire(&link, &mc) == NEON_OK);
    REQUIRE(neon_csi_init(&csi, &link) == NEON_OK);
    neon_csi_get_default_config(&config);
    config.width = 64U; config.height = 32U;
    config.pixel_packing = NEON_CSI_PACKING_UNPACKED16;
    config.dma.base_addr[0] = 0x80000000U;
    config.dma.base_addr[1] = 0x80010000U;
    config.dma.buffer_size_bytes = 4096U;
    REQUIRE(neon_csi_configure(&csi, &config) == NEON_OK);
    REQUIRE(neon_csi_start(&csi) == NEON_OK);
    for (step = 0U; step < 3000U; step++) {
        unsigned op = rnd(100U), slot = rnd(2U);
        if (op < 40U && !ready[slot] && !held[slot]) {
            csi_mmio_complete(slot, next_id, (int)rnd(2U));
            next_id = next_id == 0x3fffU ? 1U : next_id + 1U;
            ready[slot] = 1; age[slot] = serial++;
        } else if (op < 70U) {
            neon_csi_frame_t frame;
            neon_status_t status = neon_csi_acquire_frame(&csi, &frame);
            if (!ready[0] && !ready[1]) { REQUIRE(status == NEON_BUSY); }
            else {
                unsigned oldest = !ready[0] ? 1U : !ready[1] ? 0U : age[0] < age[1] ? 0U : 1U;
                REQUIRE(status == NEON_OK);
                REQUIRE(frame.buffer_index == oldest);
                REQUIRE(!held[oldest]);
                ready[oldest] = 0; held[oldest] = 1;
            }
        } else if (op < 95U && held[slot]) {
            REQUIRE(neon_csi_release_frame(&csi, (uint8_t)slot) == NEON_OK);
            held[slot] = 0;
            REQUIRE(neon_csi_release_frame(&csi, (uint8_t)slot) == NEON_INVALID_PARAM);
        } else if (op >= 95U) {
            if (op & 1U) {
                fx1_mipi_sim_set_hs_active(&link, true);
                REQUIRE(neon_csi_recover(&csi) == NEON_OK);
                fx1_mipi_sim_set_hs_active(&link, false);
            } else {
                REQUIRE(neon_csi_stop(&csi) == NEON_OK);
                REQUIRE(neon_csi_start(&csi) == NEON_OK);
            }
            ready[0] = ready[1] = held[0] = held[1] = 0;
        }
    }
    REQUIRE(neon_csi_stop(&csi) == NEON_OK);
    return 0;
}
int main(void)
{
    for (unsigned seed = 1U; seed <= 100U; seed++) if (run(seed)) return EXIT_FAILURE;
    puts("PASS CSI register model: 100 seeds, 300000 operations");
    return EXIT_SUCCESS;
}
