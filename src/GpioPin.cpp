#include "GpioPin.h"
#include "stm32f0xx_hal.h"

GpioPin::GpioPin(GPIO_TypeDef* port, uint16_t pinMask)
  : _port(port), _pinMask(pinMask)
{
}

GpioPin::GpioPin(const GpioPin& other)
  : _port(other._port), _pinMask(other._pinMask)
{
}

void GpioPin::ConfigureAsInput(uint32_t pull)
{
	InitPin(GPIO_MODE_INPUT, pull, 0, 0);
}

void GpioPin::ConfigureAsOutput(bool initialValue)
{
	Write(initialValue);
	InitPin(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0);
}

void GpioPin::ConfigureAsPeripheralOutput(uint32_t alternate)
{
	InitPin(GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, alternate);
}

void GpioPin::InitPin(uint32_t mode, uint32_t pull, uint32_t speed, uint32_t alternate)
{
	GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = _pinMask;
    GPIO_InitStruct.Mode = mode;
    GPIO_InitStruct.Pull = pull;
    GPIO_InitStruct.Speed = speed;
    GPIO_InitStruct.Alternate = alternate;
    HAL_GPIO_Init(_port, &GPIO_InitStruct);
}

bool GpioPin::Read()
{
	return HAL_GPIO_ReadPin(_port, _pinMask) == GPIO_PIN_SET;
}

void GpioPin::Write(bool setToHigh)
{
	HAL_GPIO_WritePin(_port, _pinMask, setToHigh ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

