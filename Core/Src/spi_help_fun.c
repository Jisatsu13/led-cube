#include "spi_help_fun.h"

/*----------------------------------------------------------------------*/
/*---------------------SPI Data Transmit 8 Bit--------------------------*/
/*----------------------------------------------------------------------*/
void SPI_Send8Bits(SPI_TypeDef *SPIx, uint8_t data)
{
    while (!LL_SPI_IsActiveFlag_TXE(SPIx)) {}   //check flag TXE
    LL_SPI_TransmitData8(SPIx, data);           //send data
    while (!LL_SPI_IsActiveFlag_RXNE(SPIx)) {}  //check flag RXNE
    (void) SPIx->DR;                            //read data
}
/*----------------------------------------------------------------------*/
/*---------------------SPI Data Transmit 16 Bit--------------------------*/
/*----------------------------------------------------------------------*/
void SPI_Send16Bits(SPI_TypeDef *SPIx, uint16_t data)
{
    while (!LL_SPI_IsActiveFlag_TXE(SPIx)) {}   //check flag TXE
    LL_SPI_TransmitData16(SPIx, data);          //send data
    while (!LL_SPI_IsActiveFlag_RXNE(SPIx)) {}  //check flag RXNE
    (void) SPIx->DR;                            //read data
}
/*----------------------------------------------------------------------*/
/*--------------------SPI Data Receive 8 Bit----------------------------*/
/*----------------------------------------------------------------------*/
uint8_t SPI_Read8Bits(SPI_TypeDef *SPIx)
{
	while(!LL_SPI_IsActiveFlag_TXE(SPIx)) {}
    LL_SPI_TransmitData8 (SPIx, 0x00);
    while(!LL_SPI_IsActiveFlag_RXNE(SPIx)) {}
    return LL_SPI_ReceiveData8(SPIx);
}

/*----------------------------------------------------------------------*/
/*--------------------SPI Data Receive 16 Bit---------------------------*/
/*----------------------------------------------------------------------*/
uint16_t SPI_Read16Bits(SPI_TypeDef *SPIx)
{
	while(!LL_SPI_IsActiveFlag_TXE(SPIx)) {}
    LL_SPI_TransmitData16 (SPIx, 0x00);
    while(!LL_SPI_IsActiveFlag_RXNE(SPIx)) {}
    return LL_SPI_ReceiveData16(SPIx);
}



