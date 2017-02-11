#include "LightBarrier.h"

LightBarrier::LightBarrier(TIM_TypeDef* timer, unsigned timerChannel, const GpioPin& pinIn, const GpioPin &pinControlLed)
  : _timer(timer), _timerChannel(timerChannel), _pinIn(pinIn), _pinControlLed(pinControlLed)
{
}

void LightBarrier::Init()
{
	SetPwmEnabled(false);
}

bool LightBarrier::UpdateStatus()
{
	Status currentStatus = Status::UNKNOWN;
	ResetTimeout();

	if (WaitForLights(false))
	{
		SetPwmEnabled(true);
		currentStatus = WaitForLights(true) ? Status::OPEN : Status::CLOSED;
		SetPwmEnabled(false);
	}

	SetControlLed(currentStatus == Status::CLOSED);

	if (currentStatus != _lastStatus)
	{
		_lastStatus = currentStatus;
		return true;
	}
	else
	{
		return false;
	}
}

LightBarrier::Status LightBarrier::GetStatus()
{
	return _lastStatus;
}

bool LightBarrier::WaitForLights(bool awaitedStatus)
{
	while (IsLightSeen() != awaitedStatus)
	{
		if (IsTimeout())
		{
			return false;
		}
	}
	return true;
}

bool LightBarrier::IsLightSeen()
{
	return _pinIn.Read();
}

void LightBarrier::SetPwmEnabled(bool isEnabled)
{
	uint16_t oeMask = 1 << (4*(_timerChannel-1));

	if (isEnabled)
	{
		_timer->CCER |= oeMask;
	}
	else
	{
		_timer->CCER &= ~oeMask;
	}
}

void LightBarrier::ResetTimeout()
{
	_tTimeout = HAL_GetTick() + MAX_MEASUREMENT_TIME + 1;
}

bool LightBarrier::IsTimeout()
{
	return HAL_GetTick() >= _tTimeout;
}

void LightBarrier::SetControlLed(bool status)
{
	_pinControlLed.Write(!status);
}
