/* Chỉ dùng cho ELF test; không phải API của driver hoặc service. */
#ifndef PIPELINE_CHECKS_H
#define PIPELINE_CHECKS_H
#include "ftel_isp_freertos.h"
typedef void (*pipeline_check_fn_t)(bool ok, const char *name);
typedef void (*pipeline_show_fn_t)(const char *name, uint32_t value);
void pipeline_checks_before_submit(isp_config_t *cfg, ftel_isp_pipeline_request_t *request,
                                   pipeline_check_fn_t check, pipeline_show_fn_t show);
void pipeline_checks_after_submit(isp_config_t *cfg, ftel_isp_pipeline_request_t *request);
#endif
