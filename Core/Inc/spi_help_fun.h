#ifndef SPI_HELP_FUN
#define SPI_HELP_FUN

#include "stm32g0xx_ll_spi.h"
#include "stm32g0xx_ll_utils.h"

void SPI_Send8Bits(SPI_TypeDef *SPIx, uint8_t data);
void SPI_Send16Bits(SPI_TypeDef *SPIx, uint16_t data);

uint8_t SPI_Read8Bits(SPI_TypeDef *SPIx);
uint16_t SPI_Read16Bits(SPI_TypeDef *SPIx);

#endif
