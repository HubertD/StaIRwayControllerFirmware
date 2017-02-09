#pragma once

#include "Types.h"
#include "CanController.h"

class CyclicCanMessage
{
	public:
		CanMessage Msg;
		CyclicCanMessage(CanController &can, uint32_t id, uint8_t dlc, time_ms interval);
		bool IsDue(time_ms now);
		void SendNow(time_ms now);
		void SendIfDue(time_ms now);

	private:
		CanController &_can;
		time_ms _interval;
		time_ms _tLastSent = 0;

};
