#include "StaIRwayController.h"
#include "OptionBytes.h"

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
	_spiClkPin(GpioPin(GPIOA, GPIO_PIN_5)),
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
	_msgHeartbeat(_can, MakeCanId(CAN_ID_HEARTBEAT), 4, CAN_INTERVAL_HEARTBEAT),
	_msgBarrierStatus(_can, MakeCanId(CAN_ID_BARRIER_STATUS), 1, CAN_INTERVAL_BARRIER_STATUS),
	_msgTimingMaster(_can, CAN_ID_TIMING_MASTER, 1, CAN_INTERVAL_TIMING_MASTER)
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

	_actAsTimingMaster = GetTimingMasterEnabledFromConfig();
	_deviceId = GetDeviceIdFromConfig();
	_msgHeartbeat.Msg.Id = MakeCanId(CAN_ID_HEARTBEAT);
	_msgBarrierStatus.Msg.Id = MakeCanId(CAN_ID_BARRIER_STATUS);
}

void StaIRwayController::InitHardware()
{
	HAL_Init();
	ConfigureClock();
	LightBarrier::ConfigureTimer(_timer);
	LedStrip::ConfigureSpi(SPI1);
	ConfigurePins();
	_can.Configure(6, 13, 2, 1, false, false, false);
}

void StaIRwayController::ConfigureClock()
{
	__HAL_RCC_SYSCFG_CLK_ENABLE();

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

	HAL_NVIC_SetPriority(SVC_IRQn, 0, 0);
	HAL_NVIC_SetPriority(PendSV_IRQn, 0, 0);
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
	_spiClkPin.ConfigureAsPeripheralOutput(GPIO_AF0_SPI1);

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

void StaIRwayController::Run()
{
	while (true)
	{
		time_ms now = HAL_GetTick();

		SendHeartbeatIfDue(now);
		SendBarrierStatusIfDue(now);

		ProcessTimingMaster(now);
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

				_ledStrip[i].SendData();
			}
		}
	}
}

void StaIRwayController::ProcessTimingMaster(time_ms now)
{
	if (_actAsTimingMaster && _msgTimingMaster.IsDue(now))
	{
		_msgTimingMaster.Msg.Data[0] = (1<<_timingMasterNextStep);
		_msgTimingMaster.SendNow(now);

		if (++_timingMasterNextStep >= NUM_TIMING_MASTER_STEPS)
		{
			_timingMasterNextStep = 0;
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
			case CAN_ID_GET_BARRIER_STATUS:
				UpdateBarrierStatus(msg.Data[0]);
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
			case CAN_ID_SET_DEVICE_CONFIG:
				WriteDeviceConfig(msg.Data[0], msg.Data[1]!=0);
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
	return CAN_ID_BASE | (1<<_deviceId)<<8 | functionId;
}

void StaIRwayController::UpdateBarrierStatus(uint8_t barrierMask)
{
	bool statusHasChanged = false;

	for (unsigned i=0; i<NUM_LIGHT_BARRIERS; i++)
	{
		if ( (barrierMask & (1<<i)) != 0 )
		{
			statusHasChanged |= _lightBarrier[i].UpdateStatus();
		}
	}

	if (statusHasChanged)
	{
		UpdateBarrierStatusMessage();
		_msgBarrierStatus.SendNow(HAL_GetTick());
	}
}

void StaIRwayController::SendHeartbeatIfDue(time_ms now)
{
	_msgHeartbeat.Msg.Data[0] = (now >>  0) & 0xFF;
	_msgHeartbeat.Msg.Data[1] = (now >>  8) & 0xFF;
	_msgHeartbeat.Msg.Data[2] = (now >> 16) & 0xFF;
	_msgHeartbeat.Msg.Data[3] = (now >> 24) & 0xFF;
	_msgHeartbeat.SendIfDue(now);
}

void StaIRwayController::SendBarrierStatusIfDue(time_ms now)
{
	UpdateBarrierStatusMessage();
	_msgBarrierStatus.SendIfDue(now);
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

	if (_msgBarrierStatus.Msg.Data[0] != data)
	{
		_msgBarrierStatus.Msg.Data[0] = data;
		_msgBarrierStatus.SendNow(HAL_GetTick());
	}
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
			_ledStrip[i].SendData();
		}
	}
}

uint8_t StaIRwayController::GetDeviceIdFromConfig()
{
	auto data = OptionBytes::Read();
	return (data.Data[0]==0xFF) ? DEFAULT_DEVICE_ID : data.Data[0] & OPT_DEVICE_ID_MASK;
}

bool StaIRwayController::GetTimingMasterEnabledFromConfig()
{
	auto data = OptionBytes::Read();
	return (data.Data[0] != 0xFF) && ((data.Data[0] & OPT_TIMING_MASTER_MASK) != 0);
}

void StaIRwayController::WriteDeviceConfig(uint8_t device_id, bool isTimingMaster)
{
	auto data = OptionBytes::Read();
	data.Data[0] = device_id & OPT_DEVICE_ID_MASK;
	if (isTimingMaster)
	{
		data.Data[0] |= OPT_TIMING_MASTER_MASK;
	}
	OptionBytes::Write(data);
	OptionBytes::ReloadAndReset();
}
