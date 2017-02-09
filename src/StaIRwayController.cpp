#include "StaIRwayController.h"

StaIRwayController::StaIRwayController()
  : _timer(TIM1),
	_lightBarrierOutputPin {
		GpioPin(GPIOA, GPIO_PIN_8),
		GpioPin(GPIOA, GPIO_PIN_9),
		GpioPin(GPIOA, GPIO_PIN_10),
	},
	_lightBarrierInputPin {
		GpioPin(GPIOB, GPIO_PIN_3),
		GpioPin(GPIOB, GPIO_PIN_4),
		GpioPin(GPIOB, GPIO_PIN_5),
	},
	_lightBarrierControlLedPin {
		GpioPin(GPIOA, GPIO_PIN_2),
		GpioPin(GPIOA, GPIO_PIN_1),
		GpioPin(GPIOA, GPIO_PIN_0)
	},
	_lightBarrier {
		LightBarrier(_timer, 1, _lightBarrierInputPin[0], _lightBarrierControlLedPin[0]),
		LightBarrier(_timer, 2, _lightBarrierInputPin[1], _lightBarrierControlLedPin[1]),
		LightBarrier(_timer, 3, _lightBarrierInputPin[2], _lightBarrierControlLedPin[2])
	},
	_spiMosiPin(GpioPin(GPIOA, GPIO_PIN_7)),
	_ledStripOutputEnablePin {
		GpioPin(GPIOB, GPIO_PIN_0),
		GpioPin(GPIOB, GPIO_PIN_1),
		GpioPin(GPIOA, GPIO_PIN_6),
	},
	_ledStrip {
		LedStrip(SPI1, _ledStripOutputEnablePin[0]),
		LedStrip(SPI1, _ledStripOutputEnablePin[1]),
		LedStrip(SPI1, _ledStripOutputEnablePin[2])
	},
	_canRxPin(GpioPin(GPIOA, GPIO_PIN_11)),
	_canTxPin(GpioPin(GPIOA, GPIO_PIN_12)),
	_can(CAN),
	_msgHeartbeat(_can, MakeCanId(CAN_ID_HEARTBEAT), 4, 1000),
	_msgBarrierStatus(_can, MakeCanId(CAN_ID_MEASUREMENT_RESULT), 1, 1000)
{
}

void StaIRwayController::Init()
{
	InitHardware();

	for (unsigned i=0; i<NUM_LIGHT_BARRIERS; i++)
	{
		_lightBarrier[i].Init();
	}

	for (unsigned i=0; i<NUM_LED_STRIPS; i++)
	{
		_ledStrip[i].Init();
	}
}

void StaIRwayController::InitHardware()
{
	HAL_Init();
	ConfigureClock();
	ConfigureTimer();
	ConfigurePins();
	LedStrip::ConfigureSpi(SPI1);
	_can.Configure(6, 13, 2, 1, false, false, false);
}

void StaIRwayController::ConfigureClock()
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
	RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);

	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
	__HAL_RCC_CAN1_CLK_ENABLE();
}

void StaIRwayController::ConfigurePins()
{
	for (unsigned i=0; i<NUM_LIGHT_BARRIERS; i++)
	{
		_lightBarrierInputPin[i].ConfigureAsInput(GPIO_PULLUP);
		_lightBarrierOutputPin[i].ConfigureAsPeripheralOutput(GPIO_AF2_TIM1);
	}

	_spiMosiPin.ConfigureAsPeripheralOutput(GPIO_AF0_SPI1);
	for (unsigned i=0; i<NUM_LED_STRIPS; i++)
	{
		_ledStripOutputEnablePin[i].ConfigureAsOutput(true);
	}

	_canRxPin.ConfigureAsPeripheralOutput(GPIO_AF4_CAN);
	_canTxPin.ConfigureAsPeripheralOutput(GPIO_AF4_CAN);

	for (unsigned i=0; i<NUM_LEDS; i++)
	{
		_lightBarrierControlLedPin[i].ConfigureAsOutput(true);
	}
}

void StaIRwayController::ConfigureTimer()
{
	_timer->PSC = TIMER_PRESCALE;

	/* set timer interval */
	_timer->ARR = TIMER_INTERVAL-1;

	/* set pulse width to 50% */
	_timer->CCR1 = TIMER_INTERVAL/2;
	_timer->CCR2 = TIMER_INTERVAL/2;
	_timer->CCR3 = TIMER_INTERVAL/2;

	/* configure channels to PWM mode 1 */
	_timer->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
	_timer->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;
	_timer->CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2;

	/* set master output enable */
	_timer->BDTR |= TIM_BDTR_MOE;

	/* enable timer */
	_timer->CR1 = TIM_CR1_CEN;
}

void StaIRwayController::Run()
{
/*	_lightBarrierOutputPin[0].ConfigureAsOutput(true);
	while (true)
	{
		_lightBarrierOutputPin[0].Write(true);
		_lightBarrierOutputPin[0].Write(false);
	} */

	/*
	_lightBarrier[0].SetPwmEnabled(true);
	_lightBarrier[1].SetPwmEnabled(false);
	_lightBarrier[2].SetPwmEnabled(true);
	while (true);
	 */

	while (true)
	{
		time_ms now = HAL_GetTick();

		_msgHeartbeat.Msg.Data[0] = (now >>  0) & 0xFF;
		_msgHeartbeat.Msg.Data[1] = (now >>  8) & 0xFF;
		_msgHeartbeat.Msg.Data[2] = (now >> 16) & 0xFF;
		_msgHeartbeat.Msg.Data[3] = (now >> 24) & 0xFF;
		_msgHeartbeat.SendIfDue(now);

		UpdateBarrierStatusMessage();
		_msgBarrierStatus.SendIfDue(now);

		ProcessCanMessages();

		if (_demoMode)
		{
			for (unsigned i=0; i<NUM_LIGHT_BARRIERS; i++)
			{
				_lightBarrier[i].UpdateStatus();
				if (_lightBarrier[i].GetStatus() == LightBarrier::Status::CLOSED)
				{
					_ledStrip[i].SetAllColor(0xFF0000);
				}
				else
				{
					_ledStrip[i].SetAllColor(0x00FF00);
				}
				_ledStrip[i].Update();
			}
		}
	}
}

void StaIRwayController::ProcessCanMessages()
{
	CanMessage msg;
	while (_can.Receive(msg))
	{
		if (!CanIdMatchesDeviceId(msg.Id))
		{
			continue;
		}

		_demoMode = false;

		switch (msg.Id & 0x000000FF)
		{
			case CAN_ID_TRIGGER_MEASUREMENT:
				TriggerMeasurement(msg.Data[0]);
				break;
			case CAN_ID_SET_LED:
				SetLed(msg.Data[0], msg.Data[1], msg.Data[2], msg.Data[3], msg.Data[4]);
				break;
			case CAN_ID_SET_ALL_LEDS:
				SetAllLeds(msg.Data[0], msg.Data[1], msg.Data[2], msg.Data[3]);
				break;
			case CAN_ID_UPDATE_LEDS:
				UpdateLeds(msg.Data[0]);
				break;
			default:
				break;
		}
	}
}

bool StaIRwayController::CanIdMatchesDeviceId(uint32_t id)
{
	if ( (id & CAN_ID_BASE_MASK) != CAN_ID_BASE )
	{
		return false;
	}

	uint8_t deviceMask = (id & 0x0000FF00) >> 8;
	return (deviceMask & (1<<_deviceId)) != 0;
}

uint32_t StaIRwayController::MakeCanId(uint32_t functionId)
{
	return CAN_ID_BASE | (_deviceId<<8) | functionId;
}


void StaIRwayController::TriggerMeasurement(uint8_t barrierMask)
{
	bool shouldSendMessage = false;

	for (unsigned i=0; i<NUM_LIGHT_BARRIERS; i++)
	{
		if ( (barrierMask & (1<<i)) != 0 )
		{
			if (_lightBarrier[i].UpdateStatus())
			{
				shouldSendMessage = true;
			}
		}
	}

	if (shouldSendMessage)
	{
		UpdateBarrierStatusMessage();
		_msgBarrierStatus.SendNow(HAL_GetTick());
	}
}

void StaIRwayController::UpdateBarrierStatusMessage()
{
	uint8_t data = 0;

	for (unsigned i=0; i<NUM_LIGHT_BARRIERS; i++)
	{
		if (_lightBarrier[i].GetStatus()==LightBarrier::Status::CLOSED)
		{
			data |= (1<<i);
		}
	}

	_msgBarrierStatus.Msg.Data[0] = data;
}

void StaIRwayController::SetLed(uint8_t stripMask, uint8_t led, uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t color = LedStrip::MakeColor(r, g, b);
	for (unsigned i=0; i<NUM_LED_STRIPS; i++)
	{
		if ( (stripMask & (1<<i)) != 0 )
		{
			_ledStrip[i].SetLedColor(led, color);
		}
	}
}

void StaIRwayController::SetAllLeds(uint8_t stripMask, uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t color = LedStrip::MakeColor(r, g, b);
	for (unsigned i=0; i<NUM_LED_STRIPS; i++)
	{
		if ( (stripMask & (1<<i)) != 0 )
		{
			_ledStrip[i].SetAllColor(color);
		}
	}
}

void StaIRwayController::UpdateLeds(uint8_t stripMask)
{
	for (unsigned i=0; i<NUM_LED_STRIPS; i++)
	{
		if ( (stripMask & (1<<i)) != 0 )
		{
			_ledStrip[i].Update();
		}
	}
}
