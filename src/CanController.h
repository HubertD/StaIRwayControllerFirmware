#pragma once

#include "Types.h"

struct CanMessage
{
	uint32_t Id = 0;
	uint8_t Dlc = 0;
	uint8_t Data[8] = {0};
};

class CanController
{
	public:
		static const uint32_t CAN_ID_EXTENDED = 0x80000000;
		static const uint32_t CAN_ID_RTR = 0x40000000;

		static const uint32_t CAN_MASK_STANDARD = 0x7FF;
		static const uint32_t CAN_MASK_EXTENDED = 0x1FFFFFFF;

		CanController(CAN_TypeDef *can);
		void Configure(uint16_t brp, uint8_t phase_seg1, uint8_t phase_seg2, uint8_t sjw, bool loop_back, bool listen_only, bool one_shot);
		bool Receive(CanMessage &msg);
		bool Send(const CanMessage &msg);

	private:
		CAN_TypeDef *_can;

		bool IsRxPending();
		CAN_TxMailBox_TypeDef *FindFreeTxMailbox();
};
