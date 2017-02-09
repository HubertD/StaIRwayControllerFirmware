#include "CyclicCanMessage.h"

CyclicCanMessage::CyclicCanMessage(CanController& can, uint32_t id, uint8_t dlc, time_ms interval)
	: _can(can), _interval(interval)
{
	Msg.Id = id;
	Msg.Dlc = dlc;
}

bool CyclicCanMessage::IsDue(time_ms now)
{
	return now >= (_tLastSent + _interval);
}

void CyclicCanMessage::SendNow(time_ms now)
{
	_tLastSent = now;
	_can.Send(Msg);
}

void CyclicCanMessage::SendIfDue(time_ms now)
{
	if (IsDue(now))
	{
		SendNow(now);
	}
}
