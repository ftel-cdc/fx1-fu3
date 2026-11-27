#ifndef FX1_ISP_BACKEND_H
#define FX1_ISP_BACKEND_H
#include "isp/neon_isp_session.h"
#include "fx1_isp_irq.h"
/* Minimal, versioned scalar profile. All table consumers and statistics disabled.
 * Demosaic uses INIT scalar defaults, CSC BT.601 limited range. No legacy blob
 * adapter; arbitrary CSR writes and uninitialised SRAM are deliberately absent. */
#define FX1_ISP_SCALAR_PROFILE_ABI 1u
typedef struct { uint32_t abi, wb_r, wb_g, wb_b, digital_gain; } fx1_isp_scalar_profile_t;
#define FX1_ISP_SCALAR_PROFILE_INITIALIZER {FX1_ISP_SCALAR_PROFILE_ABI, 256u, 256u, 256u, 256u}
typedef struct {
    fx1_isp_io_t io;
    /* Cache hooks required even on coherent/no-cache platforms (explicit no-op).
     * prepare: clean RAW, clean/invalidate output before DMA. complete: invalidate
     * retired output. Called on whole configured spans, never from ISR. */
    void (*prepare)(void *, uint32_t address, uint32_t bytes, bool output);
    void (*complete)(void *, uint32_t address, uint32_t bytes);
    /* attach runs after IP masked with stable context. detach MUST synchronously
     * disable PLIC/remove handler on hart 0 before returning. release invokes it
     * before dropping the shared claim, including UNSUPPORTED rollback. */
    bool (*attach)(void *);
    void (*detach)(void *);
} fx1_isp_backend_port_t;
typedef struct {
    fx1_isp_backend_port_t port;
    const void *owner;
    neon_isp_session_config_t config;
    neon_isp_session_transaction_t transaction;
    neon_isp_hw_snapshot_t evidence;
    uint32_t version, tune_id, capabilities, published, submitted, recovery_sequence, stop_sequence, setup_sequence;
    uint32_t tags[4];
    uint32_t stats_discarded[3], stats_frame[3], stats_profile[3];
    uint32_t stream_error, stream_drops, lsc_error, ee_status;
    unsigned phase;
    uint8_t bank, head;
    bool attached, stopping, reset_requested, faulted, recovery_valid;
} fx1_isp_backend_t;
neon_camera_result_t fx1_isp_backend_init(fx1_isp_backend_t *, const fx1_isp_backend_port_t *);
const neon_isp_session_backend_t *fx1_isp_backend_ops(void);
#endif
