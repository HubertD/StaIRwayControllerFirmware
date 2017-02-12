#include "OptionBytes.h"

uint16_t OptionBytes::ReadUserData()
{
	Data_t data = Read();
	return (data.Data[1] << 8) | data.Data[0];
}

bool OptionBytes::WriteUserData(uint16_t data)
{
	Data_t nextData = Read();
	nextData.Data[0] = (data & 0xFF);
	nextData.Data[1] = ((data>>8) & 0xFF);
	return Write(nextData);
}

void OptionBytes::ReloadAndReset()
{
	FLASH->CR |= FLASH_CR_OBL_LAUNCH;
	WaitReady();
}

OptionBytes::Data_t OptionBytes::Read()
{
	OB_TypeDef *ob = (OB_TypeDef*)OB_BASE;
	Data_t retval;
	retval.RDP = ob->RDP & 0xFF;
	retval.USER = ob->USER & 0xFF;
	retval.Data[0] = ob->DATA0 & 0xFF;
	retval.Data[1] = ob->DATA1 & 0xFF;
	retval.WRP[0] = ob->WRP0 & 0xFF;
#ifdef OB_WRP0_WRP1
	retval.WRP[1] = ob->WRP1 & 0xFF;
#endif
#ifdef OB_WRP0_WRP2
	retval.WRP[2] = ob->WRP2 & 0xFF;
#endif
#ifdef OB_WRP0_WRP3
	retval.WRP[3] = ob->WRP3 & 0xFF;
#endif
	return retval;
}

bool OptionBytes::Write(const OptionBytes::Data_t &data)
{
	if (IsDataEqual(Read(), data))
	{
		return true; /* nothing to do */
	}

	if (!IsErased())
	{
		Erase();
	}

	return PerformWriteOperation(data);
}

bool OptionBytes::PerformWriteOperation(const Data_t& data)
{
	OB_TypeDef *ob = (OB_TypeDef*)OB_BASE;

	WaitReady();

	UnlockFlashControlRegister();
	UnlockOptionByteWrite();

	FLASH->CR |= FLASH_CR_OPTPG;

	bool retval = true;
	retval &= WriteByte(&(ob->RDP), data.RDP);
	retval &= WriteByte(&(ob->USER), data.USER);
	retval &= WriteByte(&(ob->DATA0), data.Data[0]);
	retval &= WriteByte(&(ob->DATA1), data.Data[1]);
	retval &= WriteByte(&(ob->WRP0), data.WRP[0]);
#ifdef OB_WRP0_WRP1
	retval &= WriteByte(&(ob->WRP1), data.WRP[1]);
#endif
#ifdef OB_WRP0_WRP2
	retval &= WriteByte(&(ob->WRP2), data.WRP[2]);
#endif
#ifdef OB_WRP0_WRP3
	retval &= WriteByte(&(ob->WRP3), data.WRP[3]);
#endif

	FLASH->CR &= ~FLASH_CR_OPTPG;

	LockOptionByteWrite();
	LockFlashControlRegister();

	return retval;
}

bool OptionBytes::WriteByte(volatile uint16_t *addr, uint8_t value)
{
	*addr = value;
	WaitReady();

	bool success = (FLASH->SR & FLASH_SR_EOP) != 0;
	FLASH->SR = FLASH_SR_EOP;

	return success;
}

bool OptionBytes::IsErased()
{
	Data_t data = Read();
	return (data.RDP==0xFF)
		&& (data.USER==0xFF)
		&& (data.Data[0]==0xFF)
		&& (data.Data[1]==0xFF)
		&& (data.WRP[0]==0xFF)
		&& (data.WRP[1]==0xFF)
		&& (data.WRP[2]==0xFF)
		&& (data.WRP[3]==0xFF);
}

bool OptionBytes::Erase()
{
	WaitReady();

	UnlockFlashControlRegister();
	UnlockOptionByteWrite();

	FLASH->CR |= FLASH_CR_OPTER;
	FLASH->CR |= FLASH_CR_STRT;
	WaitReady();

	bool success = (FLASH->SR & FLASH_SR_EOP) != 0;
	FLASH->SR = FLASH_SR_EOP;
	FLASH->CR &= ~FLASH_CR_OPTER;

	LockOptionByteWrite();
	LockFlashControlRegister();

	return success && IsErased();
}

inline void OptionBytes::WaitReady()
{
	while ( IsFlashBusy() );
}

bool OptionBytes::IsDataEqual(const Data_t& data1, const Data_t& data2)
{
	return (data1.RDP == data2.RDP)
		&& (data1.USER == data2.USER)
		&& (data1.Data[0] == data2.Data[0])
		&& (data1.Data[1] == data2.Data[1])
		&& (data1.WRP[0] == data2.WRP[0])
#ifdef OB_WRP0_WRP1
		&& (data1.WRP[1] == data2.WRP[1])
#endif
#ifdef OB_WRP0_WRP2
		&& (data1.WRP[2] == data2.WRP[2])
#endif
#ifdef OB_WRP0_WRP3
		&& (data1.WRP[3] == data2.WRP[3])
#endif
	;
}

inline bool OptionBytes::IsFlashBusy()
{
	return (FLASH->SR & FLASH_SR_BSY) != 0;
}

void OptionBytes::UnlockFlashControlRegister()
{
	WaitReady();
	if ((FLASH->CR & FLASH_CR_LOCK) != 0)
	{
		FLASH->KEYR = FLASH_KEY1;
		FLASH->KEYR = FLASH_KEY2;
	}
}

void OptionBytes::LockFlashControlRegister()
{
	WaitReady();
	FLASH->CR |= FLASH_CR_LOCK;
}

void OptionBytes::UnlockOptionByteWrite()
{
	WaitReady();
	if ((FLASH->CR & FLASH_CR_OPTWRE) == 0)
	{
		FLASH->OPTKEYR = FLASH_OPTKEY1;
		FLASH->OPTKEYR = FLASH_OPTKEY2;
	}
}

void OptionBytes::LockOptionByteWrite()
{
	WaitReady();
	FLASH->CR &= ~FLASH_CR_OPTWRE;
}

