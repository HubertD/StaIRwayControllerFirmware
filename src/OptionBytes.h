#pragma once

#include "Types.h"

class OptionBytes
{
	public:
		struct Data_t
		{
			uint8_t RDP;
			uint8_t USER;
			uint8_t Data[2];
			uint8_t WRP[4];
		};

		static Data_t Read();
		static bool Write(const Data_t &data);
		static bool Erase();
		static bool IsErased();
		static void ReloadAndReset();

		static uint16_t ReadUserData();
		static bool WriteUserData(uint16_t data);

	private:
		static bool PerformWriteOperation(const Data_t &data);
		static bool WriteByte(volatile uint16_t *addr, uint8_t value);
		static bool IsDataEqual(const Data_t &data1, const Data_t &data2);

		static void UnlockFlashControlRegister();
		static void LockFlashControlRegister();
		static void UnlockOptionByteWrite();
		static void LockOptionByteWrite();
		static void WaitReady();
		static bool IsFlashBusy();

};
