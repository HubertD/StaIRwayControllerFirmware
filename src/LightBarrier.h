#pragma once

#include "Types.h"
#include "GpioPin.h"

class LightBarrier
{
	public:
		enum class Status
		{
			OPEN,
			CLOSED,
			UNKNOWN,
			TIMEOUT
		};

		LightBarrier(TIM_TypeDef* timer, unsigned timerChannel, const GpioPin &pinIn, const GpioPin &pinControlLed);
		void Init();
		bool UpdateStatus();
		Status GetStatus();
		bool IsLightSeen();
		void SetPwmEnabled(bool isEnabled);
		void SetControlLed(bool status);

	private:
		static const time_ms MAX_MEASUREMENT_TIME = 5;

		TIM_TypeDef* _timer;
		unsigned _timerChannel;

		GpioPin _pinIn;
		GpioPin _pinControlLed;
		time_ms _tTimeout = 0;

		Status _lastStatus = Status::UNKNOWN;

		bool WaitForLights(bool awaitedStatus);


		void ResetTimeout();
		bool IsTimeout();
};
