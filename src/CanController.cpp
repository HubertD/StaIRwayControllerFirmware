#include "CanController.h"

CanController::CanController(CAN_TypeDef *can)
  : _can(can)
{
}

void CanController::Configure(uint16_t brp, uint8_t phase_seg1, uint8_t phase_seg2, uint8_t sjw, bool loop_back, bool listen_only, bool one_shot)
{
	brp = brp & 0x3FF;

	uint32_t mcr = CAN_MCR_INRQ
				 | CAN_MCR_ABOM
			     | CAN_MCR_TXFP
				 | (one_shot ? CAN_MCR_NART : 0);

	uint32_t btr = ((uint32_t)(sjw-1)) << 24
  			     | ((uint32_t)(phase_seg1-1)) << 16
			     | ((uint32_t)(phase_seg2-1)) << 20
			     | (brp - 1)
				 | (loop_back ? CAN_MODE_LOOPBACK : 0)
				 | (listen_only ? CAN_MODE_SILENT : 0);

	// Reset CAN peripheral
	_can->MCR |= CAN_MCR_RESET;
	while((_can->MCR & CAN_MCR_RESET) != 0); // reset bit is set to zero after reset
	while((_can->MSR & CAN_MSR_SLAK) == 0);  // should be in sleep mode after reset

	_can->MCR |= CAN_MCR_INRQ ;
	while((_can->MSR & CAN_MSR_INAK) == 0);

	_can->MCR = mcr;
	_can->BTR = btr;

	_can->MCR &= ~CAN_MCR_INRQ;
	while((_can->MSR & CAN_MSR_INAK) != 0);

	uint32_t filter_bit = 0x00000001;
	_can->FMR |= CAN_FMR_FINIT;
	_can->FMR &= ~CAN_FMR_CAN2SB;
	_can->FA1R &= ~filter_bit;        // disable filter
	_can->FS1R |= filter_bit;         // set to single 32-bit filter mode
	_can->FM1R &= ~filter_bit;        // set filter mask mode for filter 0
	_can->sFilterRegister[0].FR1 = 0; // filter ID = 0
	_can->sFilterRegister[0].FR2 = 0; // filter Mask = 0
	_can->FFA1R &= ~filter_bit;       // assign filter 0 to FIFO 0
	_can->FA1R |= filter_bit;         // enable filter
	_can->FMR &= ~CAN_FMR_FINIT;
}

bool CanController::IsRxPending()
{
	return ((_can->RF0R & CAN_RF0R_FMP0) != 0);
}

bool CanController::Receive(CanMessage& msg)
{
	if (!IsRxPending())
	{
		return false;
	}

	CAN_FIFOMailBox_TypeDef *fifo = &_can->sFIFOMailBox[0];

	if (fifo->RIR &  CAN_RI0R_IDE)
	{
		msg.Id =  0x80000000 | ((fifo->RIR >> 3) & 0x1FFFFFFF);
	}
	else
	{
		msg.Id = (fifo->RIR >> 21) & 0x7FF;
	}

	if (fifo->RIR & CAN_RI0R_RTR)
	{
		msg.Id |= 0x40000000;
	}

	msg.Dlc = fifo->RDTR & CAN_RDT0R_DLC;

	msg.Data[0] = (fifo->RDLR >>  0) & 0xFF;
	msg.Data[1] = (fifo->RDLR >>  8) & 0xFF;
	msg.Data[2] = (fifo->RDLR >> 16) & 0xFF;
	msg.Data[3] = (fifo->RDLR >> 24) & 0xFF;
	msg.Data[4] = (fifo->RDHR >>  0) & 0xFF;
	msg.Data[5] = (fifo->RDHR >>  8) & 0xFF;
	msg.Data[6] = (fifo->RDHR >> 16) & 0xFF;
	msg.Data[7] = (fifo->RDHR >> 24) & 0xFF;

	_can->RF0R |= CAN_RF0R_RFOM0; // release FIFO

	return true;
}

bool CanController::Send(const CanMessage& msg)
{
	CAN_TxMailBox_TypeDef *mb = FindFreeTxMailbox();
	if (mb == nullptr)
	{
		return false;
	}

	/* first, clear transmission request */
	mb->TIR &= CAN_TI0R_TXRQ;

	if (msg.Id & CAN_ID_EXTENDED)
	{
		mb->TIR = CAN_ID_EXT | (msg.Id & CAN_MASK_EXTENDED) << 3;
	}
	else
	{
		mb->TIR = (msg.Id & CAN_MASK_STANDARD) << 21;
	}

	if (msg.Id & CAN_ID_RTR)
	{
		mb->TIR |= CAN_RTR_REMOTE;
	}

	mb->TDTR &= 0xFFFFFFF0;
	mb->TDTR |= msg.Dlc & 0x0F;

	mb->TDLR =
		  ( msg.Data[3] << 24 )
		| ( msg.Data[2] << 16 )
		| ( msg.Data[1] <<  8 )
		| ( msg.Data[0] <<  0 );

	mb->TDHR =
		  ( msg.Data[7] << 24 )
		| ( msg.Data[6] << 16 )
		| ( msg.Data[5] <<  8 )
		| ( msg.Data[4] <<  0 );

	/* request transmission */
	mb->TIR |= CAN_TI0R_TXRQ;

	return true;
}

CAN_TxMailBox_TypeDef* CanController::FindFreeTxMailbox()
{
	uint32_t tsr = _can->TSR;
	if ( tsr & CAN_TSR_TME0 ) { return &_can->sTxMailBox[0]; }
	if ( tsr & CAN_TSR_TME1 ) { return &_can->sTxMailBox[1]; }
	if ( tsr & CAN_TSR_TME2 ) { return &_can->sTxMailBox[2]; }
	return nullptr;
}
