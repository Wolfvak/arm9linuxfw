#pragma once

#include "common.h"

/** reset PXI hardware to a known state */
void pxiReset(void);

/** check receive & send FIFO status */
uint pxiRecvEmpty(void);
uint pxiSendFull(void);

/** receive and send data through the PXI FIFO */
u32 pxiRecvMsg(void);
void pxiSendMsg(u32 msg);

/** manipulate the PXI SYNC block */
u8 pxiRecvSync(void);
void pxiSendSync(u8 data);
void pxiTriggerSync(void);
