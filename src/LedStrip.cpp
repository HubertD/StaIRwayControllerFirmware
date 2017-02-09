#include "LedStrip.h"

void LedStrip::ConfigureSpi(SPI_TypeDef* spi)
{
	SPI_HandleTypeDef hspi;
	hspi.Instance = spi;
	hspi.Init.Mode = SPI_MODE_MASTER;
	hspi.Init.Direction = SPI_DIRECTION_2LINES;
	hspi.Init.DataSize = SPI_DATASIZE_12BIT;
	hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi.Init.NSS = SPI_NSS_SOFT;
	hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi.Init.CRCPolynomial = 7;
	hspi.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	hspi.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	HAL_SPI_Init(&hspi);
	__HAL_SPI_ENABLE(&hspi);
}

LedStrip::LedStrip(SPI_TypeDef *spi, const GpioPin &outputEnablePin)
  : _spi(spi),
	_outputEnablePin(outputEnablePin)
{
}

void LedStrip::Init()
{
	SetOutputEnabled(false);
	SetAllColor(0x000000);
	Update();
}

void LedStrip::SetAllColor(uint32_t color)
{
	for (unsigned i=0; i<NUM_LEDS; i++)
	{
		SetLedColor(i, color);
	}
}

void LedStrip::SetLedColor(unsigned led, uint32_t color)
{
	if (led<NUM_LEDS)
	{
		_buffer[3*led+0] = (color >>  8);
		_buffer[3*led+1] = (color >> 16);
		_buffer[3*led+2] = (color >>  0);
	}
}

void LedStrip::Update()
{
	SetOutputEnabled(true);
	for (unsigned i=0; i<BUFFER_SIZE; i++)
	{
		WriteByte(_buffer[i]);
	}
	WaitNotBusy();
	SetOutputEnabled(false);
	HAL_Delay(T_RESET);
}

void LedStrip::SetOutputEnabled(bool enable)
{
	_outputEnablePin.Write(!enable);
}

inline void LedStrip::WriteByte(uint8_t data)
{
	WriteBit((data & 0x80) != 0);
	WriteBit((data & 0x40) != 0);
	WriteBit((data & 0x20) != 0);
	WriteBit((data & 0x10) != 0);
	WriteBit((data & 0x08) != 0);
	WriteBit((data & 0x04) != 0);
	WriteBit((data & 0x02) != 0);
	WriteBit((data & 0x01) != 0);
}

inline void LedStrip::WriteBit(bool isHigh)
{
	WaitTxEmpty();
	_spi->DR = isHigh ? SPI_DATA_ONE : SPI_DATA_ZERO;
}

inline void LedStrip::WaitTxEmpty()
{
	while ((_spi->SR & SPI_SR_TXE_Msk) != SPI_SR_TXE_Msk);
}

uint32_t LedStrip::MakeColor(uint8_t r, uint8_t g, uint8_t b)
{
	return (r<<16) | (g<<8) | b;
}

inline void LedStrip::WaitNotBusy()
{
	while ((_spi->SR & SPI_SR_BSY_Msk) == SPI_SR_BSY_Msk);
}
