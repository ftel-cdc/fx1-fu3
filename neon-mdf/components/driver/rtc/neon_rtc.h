/**
 * @file neon_rtc.h
 * @brief Public declarations for RTC.
 * @details
 * * NEON_RTC - API hour/date decimal, backend switches to BCD according to FU2.yaml. year uses full
 * four digits; SetDate checks the date in month and leap year.
 *
 * NOT done yet: Alarm/interrupt channel/Stopwatch (true in header but not needed for use-case
 * read/write basic hours - added later if app needs it).
 *
 * Source: inc_fu2/fu2_dti_rtc.h (DTI RTC).
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_RTC_H__
#define __NEON_RTC_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_types.h"
#include "mcu_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct {
    uint32_t clock_frequency_hz; /* frequency clock granted to RTC - 0 for self declaration prescaler sees because of self calculation */
    uint32_t prescaler;          /* cpsr_div [25:0], only used when clock_frequency_hz=0. True default: freq/100 */
} neon_rtc_config_t;

typedef struct {
    neon_field8_t hour;   /* 0-23, normal decimal value (NOT BCD - driver self convert) */
    neon_field8_t minute; /* 0-59 */
    neon_field8_t second; /* 0-59 */
} neon_rtc_time_t;

typedef struct {
    neon_field16_t year; /* Full year 1..9999, e.g. 2026 */
    neon_field8_t month; /* 1-12 */
    neon_field8_t date;  /* 1-31 */
    neon_field8_t day;   /* 1=Monday..7=Sunday - ONLY read, hardware calculates itself when writing year/month/date */
} neon_rtc_date_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Initialize RTC.
 * @param[in] instance Peripheral register instance.
 * @param[in] init Initialization settings.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_rtc_init(RTC_TypeDef *instance, const neon_rtc_config_t *init);
/**
 * @brief Deinitialize RTC.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_rtc_deinit(RTC_TypeDef *instance);

/**
 * @brief Set RTC time.
 * @param[in] instance Peripheral register instance.
 * @param[in] time RTC time fields.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_rtc_set_time(RTC_TypeDef *instance, const neon_rtc_time_t *time);
/**
 * @brief Read RTC time.
 * @param[in] instance Peripheral register instance.
 * @param[out] time RTC time fields.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_rtc_get_time(RTC_TypeDef *instance, neon_rtc_time_t *time);
/**
 * @brief Set RTC date.
 * @param[in] instance Peripheral register instance.
 * @param[in] date RTC calendar fields.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_rtc_set_date(RTC_TypeDef *instance, const neon_rtc_date_t *date);
/**
 * @brief Read RTC date.
 * @param[in] instance Peripheral register instance.
 * @param[out] date RTC calendar fields.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_rtc_get_date(RTC_TypeDef *instance, neon_rtc_date_t *date);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_RTC_H__ */
