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
		static const unsigned NUM_LEDS = 3;

		static const uint32_t CAN_ID_BASE = CanController::CAN_ID_EXTENDED | 0x13390000;
		static const uint32_t CAN_ID_BASE_MASK = 0xFFFF0000;

		static const uint32_t CAN_ID_HEARTBEAT = 0;
		static const uint32_t CAN_ID_GET_BARRIER_STATUS = 1;
		static const uint32_t CAN_ID_BARRIER_STATUS = 2;
		static const uint32_t CAN_ID_SET_LED = 3;
		static const uint32_t CAN_ID_SET_ALL_LEDS = 4;
		static const uint32_t CAN_ID_UPDATE_LEDS = 5;
		static const uint32_t CAN_ID_SET_DEVICE_ID = 6;

		static const time_ms CAN_INTERVAL_HEARTBEAT = 1000;
		static const time_ms CAN_INTERVAL_BARRIER_STATUS = 100;

		unsigned _deviceId = 0;

		TIM_TypeDef *_timer;
		GpioPin _lightBarrierOutputPin[NUM_LIGHT_BARRIERS];
		GpioPin _lightBarrierInputPin[NUM_LIGHT_BARRIERS];
		GpioPin _lightBarrierControlLedPin[NUM_LEDS];
		LightBarrier _lightBarrier[NUM_LIGHT_BARRIERS];

		GpioPin _spiMosiPin;
		GpioPin _spiClkPin;
		GpioPin _ledStripOutputEnablePin[NUM_LED_STRIPS];
		LedStrip _ledStrip[NUM_LED_STRIPS];

		GpioPin _canRxPin, _canTxPin;

		CanController _can;
		CyclicCanMessage _msgHeartbeat;
		CyclicCanMessage _msgBarrierStatus;

		bool _demoMode = true;


		void InitHardware();
		void ConfigureClock();
		void ConfigurePins();

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

		uint8_t ReadDeviceId();
		void WriteDeviceId(uint8_t device_id);
};
