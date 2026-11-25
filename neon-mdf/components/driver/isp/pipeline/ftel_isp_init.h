/*
 * Khởi tạo nguội toàn pipeline bằng API từng khối; không cấp quyền DMA hay gửi frame.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_INIT_H
#define FTEL_ISP_INIT_H
#include "bayer domain/ifmt/ftel_isp_ifmt.h"
#include "bayer domain/blc/ftel_isp_blc.h"
#include "bayer domain/lsc/ftel_isp_lsc.h"
#include "bayer domain/bpc/ftel_isp_bpc.h"
#include "bayer domain/wb/ftel_isp_wb.h"
#include "bayer domain/dg/ftel_isp_dg.h"
#include "rgb domain/demosaic/ftel_isp_demosaic.h"
#include "rgb domain/ccm/ftel_isp_ccm.h"
#include "rgb domain/gamma/ftel_isp_gamma.h"
#include "rgb domain/csc/ftel_isp_csc.h"
#include "yuv domain/gtm/ftel_isp_gtm.h"
#include "yuv domain/nr/ftel_isp_nr.h"
#include "yuv domain/ee/ftel_isp_ee.h"
#include "yuv domain/cnf/ftel_isp_cnf.h"
#include "stats/aec/ftel_isp_aec.h"
#include "stats/awb/ftel_isp_awb.h"
#include "stats/af/ftel_isp_af.h"
#include "io/output/ftel_isp_output.h"
#include "io/dma/ftel_isp_dma.h"

typedef struct {
    ifmt_config_t ifmt;
    blc_config_t blc;
    lsc_init_config_t lsc;
    bpc_config_t bpc;
    wb_config_t wb;
    dg_config_t dg;
    demosaic_config_t demosaic;
    ccm_config_t ccm;
    gamma_config_t gamma;
    csc_config_t csc;
    gtm_config_t gtm;
    nr_config_t nr;
    ee_config_t ee;
    cnf_config_t cnf;
    aec_config_t aec;
    awb_stat_config_t awb;
    af_config_t af;
    output_config_t output;
    dma_config_t dma;
} isp_config_t;

typedef enum {
    ISP_INIT_VALIDATE, ISP_INIT_ABI, ISP_INIT_IDLE, ISP_INIT_RESET,
    ISP_INIT_IFMT, ISP_INIT_BLC, ISP_INIT_LSC, ISP_INIT_BPC, ISP_INIT_WB, ISP_INIT_DG,
    ISP_INIT_DEMOSAIC, ISP_INIT_CCM, ISP_INIT_GAMMA, ISP_INIT_CSC, ISP_INIT_GTM,
    ISP_INIT_NR, ISP_INIT_EE, ISP_INIT_CNF, ISP_INIT_AEC, ISP_INIT_AWB, ISP_INIT_AF,
    ISP_INIT_OUTPUT, ISP_INIT_DMA, ISP_INIT_GEOMETRY, ISP_INIT_READY
} isp_init_stage_t;

typedef struct {
    isp_init_stage_t stage;
    isp_status_t status;
    uint32_t completed; /* Bit tương ứng stage được đặt khi bước đã hoàn tất. */
    uint16_t active_width, active_height;
    lsc_result_t lsc; /* Lưu lỗi/bước nạp LSC trước ABORT nếu init dừng tại LSC. */
} isp_init_result_t;

/*
 * Điền cấu hình trong RAM, chưa ghi phần cứng. Mặc định cung cấp các bảng/LUT,
 * tắt khối tùy chọn và bật CH0 NV12. Đổi đầu vào cần tính lại AEC/output/DMA.
 */
void isp_get_default_config(isp_config_t *cfg);
isp_status_t isp_validate_config(const isp_config_t *cfg); /* Không truy cập MMIO. */
/*
 * Bên gọi sở hữu thiết bị và bộ nhớ. Lỗi kiểm tra tham số/ABI/busy/profile/lease không ghi thanh
 * ghi. Sau reset, lỗi giữ lại stage; DMA chưa cấp quyền, enable tạm để tính hình học được xóa.
 * Tuning đã ghi không tự hoàn tác. Sửa nguyên nhân rồi init lại. wait=NULL dùng
 * giới hạn 100000 lần thăm dò cho reset/hình học. result=NULL dùng kết quả tạm.
 * Luồng: kiểm tra -> ABI/rỗi -> reset -> cấu hình khối -> đầu ra/DMA -> hình học -> READY.
 */
isp_status_t isp_init(const isp_config_t *cfg, const isp_wait_t *wait, isp_init_result_t *result);
const char *isp_init_stage_name(isp_init_stage_t stage);
#endif
