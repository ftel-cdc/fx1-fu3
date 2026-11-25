#ifndef NEON_COMPONENT_FTEL_ISP_H
#define NEON_COMPONENT_FTEL_ISP_H

/* HAL công khai: kiểu dữ liệu và API; không chứa thanh ghi hoặc FreeRTOS. */
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FTEL_ISP_OK = 0,
    FTEL_ISP_E_CONFIG = -1,
    FTEL_ISP_E_ABI = -2,
    FTEL_ISP_E_BUSY = -3,
    FTEL_ISP_E_TIMEOUT = -4,
    FTEL_ISP_E_IO = -5,
    FTEL_ISP_E_UNSUPPORTED = -6,
    FTEL_ISP_E_STATE = -7,
    FTEL_ISP_E_NO_DATA = -8,
    FTEL_ISP_E_STALE = -9
} ftel_isp_status_t;

typedef enum { FTEL_ISP_RAW10 = 10, FTEL_ISP_RAW12 = 12 } ftel_isp_raw_depth_t;
typedef enum {
    FTEL_ISP_RGGB, FTEL_ISP_GRBG, FTEL_ISP_GBRG, FTEL_ISP_BGGR
} ftel_isp_bayer_pattern_t;
typedef enum { FTEL_ISP_AEC_AFTER_BPC, FTEL_ISP_AEC_AFTER_DG } ftel_isp_aec_source_t;

typedef struct {
    uint16_t width, height;
    ftel_isp_raw_depth_t raw_depth;
    ftel_isp_bayer_pattern_t pattern;
} ftel_isp_input_config_t;

typedef struct {
    ftel_isp_input_config_t input;
    struct { bool enable; uint16_t black_level; } blc;
    struct { bool enable; } bpc;
    struct { bool enable; uint16_t r_q8, g_q8, b_q8; } wb;
    struct { bool enable; uint16_t gain_q8; } dg;
    /* Mốc đầu chỉ bypass LSC, chưa nạp/chọn bảng calibration. */
} ftel_isp_bayer_config_t;

typedef struct {
    bool enable;
    ftel_isp_aec_source_t source;
    uint32_t context_id;
    /* Lưới zone hiện lấy mặc định theo kích thước hữu dụng sau IFMT. */
} ftel_isp_aec_config_t;

typedef struct {
    ftel_isp_bayer_config_t bayer;
    ftel_isp_aec_config_t aec;
} ftel_isp_config_t;

typedef enum {
    FTEL_ISP_INIT_VALIDATE, FTEL_ISP_INIT_ABI, FTEL_ISP_INIT_IDLE,
    FTEL_ISP_INIT_RESET, FTEL_ISP_INIT_IFMT, FTEL_ISP_INIT_BLC,
    FTEL_ISP_INIT_LSC_BYPASS, FTEL_ISP_INIT_BPC, FTEL_ISP_INIT_WB,
    FTEL_ISP_INIT_DG, FTEL_ISP_INIT_AEC, FTEL_ISP_INIT_CHECK, FTEL_ISP_INIT_READY
} ftel_isp_init_stage_t;

typedef struct {
    ftel_isp_init_stage_t stage;
    int32_t driver_status; /* Mã gốc để chẩn đoán lỗi tại hal_implements. */
    uint16_t active_width, active_height;
} ftel_isp_init_result_t;

typedef struct {
    uint32_t version, date, tune_revision, capabilities;
    uint32_t input_count, output_count, recovery_sequence;
    bool busy;
} ftel_isp_info_t;

/* Đọc thông tin IP; không reset hoặc ghi cấu hình. */
ftel_isp_status_t ftel_isp_get_info(ftel_isp_info_t *info);

/* Chỉ điền RAM: RAW12/RGGB 64x48, các khối Bayer bypass/1x, bật AEC sau DG. */
void ftel_isp_get_default_config(ftel_isp_config_t *config);

/* Init nguội phần Bayer/AEC; bên gọi sở hữu độc quyền ISP, không có frame
 * đang chạy. Kiểm tra cấu hình trước khi ghi, reset rồi cấu hình từng khối.
 * Chưa cấu hình RGB/YUV/output, không cấp DMA hoặc chạy frame.
 * result có thể NULL. Lỗi sau reset có thể để lại cấu hình một phần. */
ftel_isp_status_t ftel_isp_init_bayer_aec(const ftel_isp_config_t *config,
                                        ftel_isp_init_result_t *result);

/* Buffer DDR thuộc bên gọi, địa chỉ và stride căn 16 byte. RAW unpack uint16
 * little-endian, căn phải: RAW10 0..1023 hoặc RAW12 0..4095. bytes chứa đủ cả
 * padding theo stride. Bên gọi phải giữ cả ba buffer tới khi DMA đã dừng. */
typedef struct {
    uintptr_t address;
    uint32_t stride, bytes;
} ftel_isp_buffer_t;

typedef struct {
    ftel_isp_buffer_t raw, y, uv; /* Output CH0 NV12 chỉ để thoát dữ liệu. */
    uint32_t tag, timestamp;
} ftel_isp_frame_t;

typedef struct {
    uint32_t input_count, output_count, frame_id, tag, timestamp;
    uint32_t dma_errors; /* Snapshot gốc; bit IDMA_UNDERRUN có thể là chờ RAW kế. */
} ftel_isp_frame_result_t;

/* Một frame slot 0 sau mỗi init, polling có timeout; không IRQ/callback/AE.
 * Cần AEC enable khi init. Thành công: ISP/DMA dừng, output thuộc CPU.
 * Timeout/lỗi sau cấp VALID: chưa bảo đảm trả quyền buffer; không tái sử dụng
 * buffer hay gọi lại tùy tiện, phải xác nhận drain/reset ở nền tảng trước. */
ftel_isp_status_t ftel_isp_run_frame(const ftel_isp_frame_t *frame, uint32_t timeout_us,
                                    ftel_isp_frame_result_t *result);

#define FTEL_ISP_AEC_HIST_BINS 64u
typedef struct {
    uint32_t frame_id, context_id;
    ftel_isp_aec_source_t source;
    uint64_t sum, count, ue_count, oe_count;
    uint16_t ue_percent_q8, oe_percent_q8; /* Leased global Q7.8 percent, 100% = 0x6400. */
    uint64_t histogram[FTEL_ISP_AEC_HIST_BINS];
    bool overflow;
} ftel_isp_aec_stats_t;

/* Không chặn; acquire -> copy global/histogram -> release. Chưa có kết quả
 * trả NO_DATA; không lấy lease do bên khác đang giữ. Chỉ cập nhật stats khi
 * đọc/release thành công và frame/context/source khớp frame vừa chạy.
 * Mọi API module phải do một task/hart sở hữu và gọi tuần tự. */
ftel_isp_status_t ftel_isp_read_aec(ftel_isp_aec_stats_t *stats);

#ifdef __cplusplus
}
#endif
#endif
