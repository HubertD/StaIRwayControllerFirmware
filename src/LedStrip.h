#pragma once

#include "Types.h"
#include "GpioPin.h"

class LedStrip
{
	public:
		LedStrip(SPI_TypeDef *spi, const GpioPin &outputEnablePin);
		void Init();
		void SetLedColor(unsigned led, uint32_t color);
		void SetAllColor(uint32_t color);
		void Update();

		static void ConfigureSpi(SPI_TypeDef *spi);
		static uint32_t MakeColor(uint8_t r, uint8_t g, uint8_t b);

	private:
		static const unsigned NUM_LEDS = 48;
		static const unsigned BUFFER_SIZE = 3*NUM_LEDS;
		static const time_ms T_RESET = 1;
		static const uint16_t SPI_DATA_ONE = 0b000000001111;
		static const uint16_t SPI_DATA_ZERO = 0b000011111111;
		uint8_t _buffer[BUFFER_SIZE];
		SPI_TypeDef *_spi;
		GpioPin _outputEnablePin;

		void SetOutputEnabled(bool enable);
		void WriteByte(uint8_t data);
		void WriteBit(bool isHigh);

		void WaitTxEmpty();
		void WaitNotBusy();

};
