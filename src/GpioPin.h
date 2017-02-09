#pragma once

#include "Types.h"

class GpioPin
{
	public:
		GpioPin(GPIO_TypeDef* port, uint16_t pinMask);
		GpioPin(const GpioPin &other);

		void ConfigureAsInput(uint32_t pull);
		void ConfigureAsOutput(bool initialValue);
		void ConfigureAsPeripheralOutput(uint32_t alternate);

		bool Read();
		void Write(bool setToHigh);

	private:
		GPIO_TypeDef* _port;
		uint16_t _pinMask;

		void InitPin(uint32_t mode, uint32_t pull, uint32_t speed, uint32_t alternate);
};
