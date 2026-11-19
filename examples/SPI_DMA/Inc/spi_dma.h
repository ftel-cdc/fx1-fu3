/**
 * @file spi_dma.h
 * @brief SPI0 DMA TX/RX checks using W5500 SRAM.
 * @details
 * SPI0 TX/RX DMA tests using a W5500 socket TX SRAM buffer.
 */

#ifndef GT32U2086_EXAMPLE_SPI_DMA_H
#define GT32U2086_EXAMPLE_SPI_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Write a 32-byte pattern using DMA, verify by polling, then read using DMA.
 */
void spi_dma_run(void);

#ifdef __cplusplus
}
#endif

#endif /* GT32U2086_EXAMPLE_SPI_DMA_H */
