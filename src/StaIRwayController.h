#pragma once

#include "Types.h"
#include "LightBarrier.h"
#include "LedStrip.h"
#include "GpioPin.h"
#include "CanController.h"
#include "CyclicCanMessage.h"

class StaIRwayController
{
	public:
		StaIRwayController();
		void Init();
		void Run();

	private:

		static const unsigned NUM_LIGHT_BARRIERS = 3;
		static const unsigned NUM_LED_STRIPS = 3;

		static const uint32_t CAN_ID_BASE = CanController::CAN_ID_EXTENDED | 0x13390000;
		static const uint32_t CAN_ID_BASE_MASK = 0xFFFF0000;

		static const uint32_t CAN_ID_HEARTBEAT = 0;
		static const uint32_t CAN_ID_GET_BARRIER_STATUS = 1;
		static const uint32_t CAN_ID_BARRIER_STATUS = 2;
		static const uint32_t CAN_ID_SET_LED = 3;
		static const uint32_t CAN_ID_SET_ALL_LEDS = 4;
		static const uint32_t CAN_ID_UPDATE_LEDS = 5;
		static const uint32_t CAN_ID_SET_DEVICE_CONFIG = 6;

		static const time_ms CAN_INTERVAL_HEARTBEAT = 1000;
		static const time_ms CAN_INTERVAL_BARRIER_STATUS = 100;

		static const uint32_t CAN_ID_TIMING_MASTER = CAN_ID_BASE | 0xFF00 | CAN_ID_GET_BARRIER_STATUS;
		static const time_ms CAN_INTERVAL_TIMING_MASTER = 10;
		static const unsigned NUM_TIMING_MASTER_STEPS = 3;

		static const uint8_t DEFAULT_DEVICE_ID = 7;
		static const uint8_t OPT_DEVICE_ID_MASK = 0x07;
		static const uint8_t OPT_TIMING_MASTER_MASK = 0x80;

		TIM_TypeDef *_timer { TIM1 };
		CanController _can { CAN };

		GpioPin _spiMosiPin { GPIOA, GPIO_PIN_7 };
		GpioPin _spiClkPin { GPIOA, GPIO_PIN_5 };
		GpioPin _canRxPin { GPIOA, GPIO_PIN_11 };
		GpioPin _canTxPin { GPIOA, GPIO_PIN_12 };

		LightBarrier _lightBarrier[NUM_LIGHT_BARRIERS] {
			{ _timer, 1, { GPIOA, GPIO_PIN_8 }, GPIO_AF2_TIM1, { GPIOB, GPIO_PIN_3 }, { GPIOA, GPIO_PIN_2 } },
			{ _timer, 2, { GPIOA, GPIO_PIN_9 }, GPIO_AF2_TIM1, { GPIOB, GPIO_PIN_4 }, { GPIOA, GPIO_PIN_1 } },
			{ _timer, 3, { GPIOA, GPIO_PIN_10 }, GPIO_AF2_TIM1, { GPIOB, GPIO_PIN_5 }, { GPIOA, GPIO_PIN_0 } },
		};

		LedStrip _ledStrip[NUM_LED_STRIPS] {
			{ SPI1, { GPIOB, GPIO_PIN_0 } },
			{ SPI1, { GPIOB, GPIO_PIN_1 } },
			{ SPI1, { GPIOA, GPIO_PIN_6 } }
		};

		CyclicCanMessage _msgHeartbeat { _can, 0, 4, CAN_INTERVAL_HEARTBEAT };
		CyclicCanMessage _msgBarrierStatus { _can, 0, 1, CAN_INTERVAL_BARRIER_STATUS };
		CyclicCanMessage _msgTimingMaster { _can, CAN_ID_TIMING_MASTER, 1, CAN_INTERVAL_TIMING_MASTER };

		unsigned _deviceId = 0;
		bool _demoMode = true;
		bool _actAsTimingMaster = false;
		uint32_t _timingMasterNextStep = 0;

		void InitHardware();
		void ConfigureClock();
		void ConfigurePins();

		void ProcessTimingMaster(time_ms now);
		void ProcessCanMessages();
		uint32_t MakeCanId(uint32_t functionId);
		bool CanIdMatchesDeviceId(uint32_t id);
		void UpdateBarrierStatus(uint8_t barrierMask);
		void SetLed(uint8_t stripMask, uint8_t led, uint8_t r, uint8_t g, uint8_t b);
		void SetAllLeds(uint8_t stripMask, uint8_t r, uint8_t g, uint8_t b);
		void UpdateLeds(uint8_t stripMask);
		void UpdateBarrierStatusMessage();

		void SendHeartbeatIfDue(time_ms now);
		void SendBarrierStatusIfDue(time_ms now);

		uint8_t GetDeviceIdFromConfig();
		bool GetTimingMasterEnabledFromConfig();
		void WriteDeviceConfig(uint8_t device_id, bool isTimingMaster);
};
