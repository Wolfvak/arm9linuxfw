#include "asm.h"

.arm
.align 2

.global virtDevResetStub
.type virtDevResetStub, %function

.global virtDevRdCfgStub
.type virtDevRdCfgStub, %function

.global virtDevWrCfgStub
.type virtDevWrCfgStub, %function

.global virtDevPrQueueStub
.type virtDevPrQueueStub, %function

@ Very simple virtDev operation stubs
virtDevRdCfgStub:
virtDevWrCfgStub:
	mov r0, #0

virtDevResetStub:
virtDevPrQueueStub:
	bx lr
