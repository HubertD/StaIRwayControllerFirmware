#include "LightBarrier.h"


void LightBarrier::ConfigureTimer(TIM_TypeDef* timer)
{
	timer->PSC = TIMER_PRESCALE;

	/* set timer interval */
	timer->ARR = TIMER_INTERVAL-1;

	/* set pulse width to 50% */
	timer->CCR1 = TIMER_INTERVAL/2;
	timer->CCR2 = TIMER_INTERVAL/2;
	timer->CCR3 = TIMER_INTERVAL/2;

	/* configure channels to PWM mode 1 */
	timer->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
	timer->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;
	timer->CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2;

	/* set master output enable */
	timer->BDTR |= TIM_BDTR_MOE;

	/* enable timer */
	timer->CR1 = TIM_CR1_CEN;
}

LightBarrier::LightBarrier(TIM_TypeDef* timer, unsigned timerChannel, const GpioPin &pinOut, const GpioPin& pinIn, const GpioPin &pinControlLed)
  : _timer(timer), _timerChannel(timerChannel), _pinOut(pinOut), _pinIn(pinIn), _pinControlLed(pinControlLed)
{
}

void LightBarrier::Init()
{
	SetPwmEnabled(false);
	ConfigurePins();
}

void LightBarrier::ConfigurePins()
{
	_pinIn.ConfigureAsInput(GPIO_PULLUP);
	_pinControlLed.ConfigureAsOutput(true);
	_pinOut.ConfigureAsPeripheralOutput(GPIO_AF2_TIM1);
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
