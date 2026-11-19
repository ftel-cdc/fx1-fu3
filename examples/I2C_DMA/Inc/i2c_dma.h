/**
 * @file i2c_dma.h
 * @brief I2C0 DMA EEPROM example entry point.
 * @details
 * Runs a single DMA write/read comparison using an AT24C256 EEPROM.
 */

#ifndef GT32U2086_EXAMPLE_I2C_DMA_H
#define GT32U2086_EXAMPLE_I2C_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Write 16 EEPROM bytes with DMA, read them back with DMA and compare.
 * @details Overwrites EEPROM offsets 0000..000F without restoring their contents.
 */
void i2c_dma_run(void);

#ifdef __cplusplus
}
#endif

#endif /* GT32U2086_EXAMPLE_I2C_DMA_H */
