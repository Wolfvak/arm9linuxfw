#include "common.h"

#include "virt/manager.h"

#define REG_PRNG_BASE	(0x10011000)

typedef struct {
	vu32 cold;
	u32 resv1[3];

	vu32 hot;
	u32 resv2[3];

	vu16 cold_seed;
	vu16 hot_seed;
} PACKED prngRegs;

static prngRegs *getPrngRegs(void) {
	return (prngRegs*)(REG_PRNG_BASE);
}

static void hwrngProcessQueue(virtDev_s *vdev, virtQueue_s *vq) {
	virtJob_s vjob;
	prngRegs *regs = getPrngRegs();

	while(virtQueueFetchJobNew(vq, &vjob) >= 0) {
		do {
			u32 *data;
			virtDesc_s desc;
			virtQueueGetJobDesc(vq, &vjob, &desc);

			if (desc.dir == HOST_TO_VDEV)
				continue;

			data = desc.data;
			for (uint i = 0; i < (desc.length/4); i++)
				data[i] = regs->hot;
			virtJobAddWritten(&vjob, desc.length);
		} while(virtQueueFetchJobNext(vq, &vjob) >= 0);

		virtQueuePushJob(vq, &vjob);
	}

	virtDevNotifyHost(vdev, VIRQ_VQUEUE);
}

DECLARE_VIRTDEV(
	hwrngDevice, NULL,
	VDEV_T_HWRNG, 0, 1,
	virtDevResetStub,
	virtDevRdCfgStub, virtDevWrCfgStub,
	hwrngProcessQueue
);
