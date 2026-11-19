/**
 * @file adc_sampling.h
 * @brief ADC sampling testcase entry point.
 */

#ifndef FPGA_TERMINAL_ADC_SAMPLING_H
#define FPGA_TERMINAL_ADC_SAMPLING_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Read ADC batches of 1, 4, and 16 samples and report the results.
 */
void adc_sampling_run(void);

#ifdef __cplusplus
}
#endif

#endif /* FPGA_TERMINAL_ADC_SAMPLING_H */
