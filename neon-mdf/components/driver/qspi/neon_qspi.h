/**
 * @file neon_qspi.h
 * @brief Public declarations for QSPI.
 * @details
 * * NEON_QSPI - chế độ Indirect (lệnh + data qua FIFO phần mềm điều khiển).
 * config/DeInit/SendCommand port THẬT từ src_fu2/fu2_dti_qspi.c (team cũ,
 * BuuPQ). WriteData/ReadData/IsBusy/WaitReady là wrap trực tiếp thanh ghi
 * TX/RX/STT.BUSY thật (inline LL_QSPI_IsBusy trong header) - KHÔNG có cờ
 * FIFO full/empty riêng nào được xác nhận trong inc_fu2/fu2_dti_qspi.h
 * (chỉ có STT.BUSY), nên KHÔNG bịa thêm polling flag nào khác - caller tự
 * dùng IsBusy()/WaitReady() giữa các lần đọc/ghi.
 *
 * CHỈ điều khiển qua cửa sổ APB (DTI_QSPI = QSPI_APB_BASE) - cửa sổ AHB-Lite
 * (QSPI_CTRL, XiP memory-mapped read) KHÔNG có struct thanh ghi riêng trong
 * header team (chỉ là vùng nhớ đọc trực tiếp khi CFG.FMODE=MEM_MAPPED), xem
 * FU2.yaml peripheral QSPI_CTRL.
 *
 * Nguồn: inc_fu2/fu2_dti_qspi.h ("Dolphin Technology DTI QSPI").
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_QSPI_H__
#define __NEON_QSPI_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_types.h"
#include "mcu_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* Clock divider - SCLK = sysclk / (2^(div_index+1)), div_index = giá trị dưới đây */
#define NEON_QSPI_CLK_DIV_2 0x0U
#define NEON_QSPI_CLK_DIV_4 0x1U
#define NEON_QSPI_CLK_DIV_8 0x2U
#define NEON_QSPI_CLK_DIV_256 0x7U

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum {
    NEON_QSPI_PIN_CONFIG_AUTO = 0,
    NEON_QSPI_PIN_CONFIG_KEEP = 1,
} neon_qspi_pin_config_t;

typedef struct {
    uint32_t clock_divider; /* NEON_QSPI_CLK_DIV_* */
    /* AUTO routes SCK/SSN/D0..D3; DeInit restores the saved pad/mux bits.
     * KEEP uses the board's existing routing. Zero initialization selects AUTO. */
    neon_qspi_pin_config_t pin_config;
} neon_qspi_config_t;

/* Line count cho instruction/address/data - dùng chung 1 constant vì cùng
 * encoding thật (0=NONE,1=1LINE,2=2LINES,3=4LINES) trên cả 3 field CCR. */
#define NEON_QSPI_LINES_NONE 0x0U
#define NEON_QSPI_LINES_1 0x1U
#define NEON_QSPI_LINES_2 0x2U
#define NEON_QSPI_LINES_4 0x3U

typedef struct {
    neon_field8_t instruction;    /* opcode 8-bit gửi cho flash (vd 0x9F = Read JEDEC ID) */
    uint32_t instruction_lines;   /* NEON_QSPI_LINES_* */
    uint32_t address_lines;       /* NEON_QSPI_LINES_NONE nếu lệnh không cần địa chỉ */
    uint32_t address;             /* chỉ dùng khi address_lines != NONE, luôn 24-bit (ADDRSIZE cố định) */
    uint32_t dummy_cycles;        /* 0-31 */
    uint32_t data_lines;          /* NEON_QSPI_LINES_NONE nếu lệnh không có data phase */
    uint32_t data_length;         /* số byte data, chỉ dùng khi data_lines != NONE */
    neon_field8_t receive_enable; /* 1 = đọc data từ slave (vd Read), 0 = ghi (vd Page Program) */
} neon_qspi_command_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/** @brief Default QSPI config: divide by 2, automatic pad/mux setup. */
void neon_qspi_get_default_config(neon_qspi_config_t *config);

/**
 * @brief Initialize QSPI.
 * @param[in] instance Peripheral register instance.
 * @param[in] init Initialization settings.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_qspi_init(QSPI_TypeDef *instance, const neon_qspi_config_t *init);
/**
 * @brief Deinitialize QSPI.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_qspi_deinit(QSPI_TypeDef *instance);

/**
 * @brief Qspi send command.
 * @param[in] instance Peripheral register instance.
 * @param[in] cmd Cmd.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * config thiết lập mode 0/SDR; config và SendCommand trả BUSY khi đang transfer.
 * Ghi AR/ABR/DLR rồi CCR - ở chế độ Indirect, ghi CCR THẬT SỰ BẮT ĐẦU
 * transfer ngay (khớp comment gốc "writing CCR is the trigger"). Gọi
 * WriteData/ReadData sau đó để bơm/đọc data qua FIFO (giao tiếp SPI/QSPI
 * flash chuẩn: gửi lệnh trước, data sau/đồng thời tuỳ chip flash thật).
 */
neon_status_t neon_qspi_send_command(QSPI_TypeDef *instance, const neon_qspi_command_t *cmd);

/**
 * @brief Write QSPI data.
 * @param[in] instance Peripheral register instance.
 * @param[in] data data for this operation.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_qspi_write_data(QSPI_TypeDef *instance, uint32_t data);
/**
 * @brief Read QSPI data.
 * @param[in] instance Peripheral register instance.
 * @return The requested value or operation result.
 */
uint32_t neon_qspi_read_data(QSPI_TypeDef *instance);
/**
 * @brief Check QSPI busy.
 * @param[in] instance Peripheral register instance.
 * @return The requested value or operation result.
 */
uint32_t neon_qspi_is_busy(QSPI_TypeDef *instance);
/**
 * @brief Qspi wait ready.
 * @param[in] instance Peripheral register instance.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Blocking - poll BUSY tới khi =0 hoặc hết timeout_polls (số vòng poll).
 */
neon_status_t neon_qspi_wait_ready(QSPI_TypeDef *instance, uint32_t timeout_polls);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_QSPI_H__ */
