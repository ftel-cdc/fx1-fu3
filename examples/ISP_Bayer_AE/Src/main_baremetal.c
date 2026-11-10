/* Mốc 1–3: bare-metal, probe ISP rồi init Bayer/AEC, chưa chạy frame. */
#include "ftel_isp.h"
#include "fx1_fw.h"

static unsigned checks;
static void check(bool ok, const char *name)
{
    ++checks;
    fx1_puts(ok ? "[baremetal] PASS " : "[baremetal] FAIL ");
    fx1_puts(name); fx1_putc('\n');
    if (!ok) fx1_fail(0xea00u + checks);
}

int main(void)
{
    ftel_isp_info_t info;
    ftel_isp_config_t config;
    ftel_isp_init_result_t result;
    check(ftel_isp_get_info(&info) == FTEL_ISP_OK, "ISP responds through HAL");
    fx1_log_hex("ISP_VERSION", info.version);
    fx1_log_hex("ISP_DATE", info.date);
    fx1_log_hex("ISP_TUNE_REV", info.tune_revision);
    check(!info.busy, "cold ISP idle");
    ftel_isp_get_default_config(&config);
    check(ftel_isp_init_bayer_aec(&config, &result) == FTEL_ISP_OK &&
          result.stage == FTEL_ISP_INIT_READY, "Bayer -> AEC init ready");
    check(result.active_width == 64 && result.active_height == 48, "active geometry 64x48");
    check(ftel_isp_get_info(&info) == FTEL_ISP_OK && !info.busy, "init does not start frame");
    fx1_puts("BAREMETAL_INIT PASS checks="); fx1_putdec(checks); fx1_putc('\n');
    fx1_pass();
}
