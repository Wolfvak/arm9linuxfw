#include "common.h"

#include "arm/arm.h"
#include "hw/pxi.h"

#include "virt/irq.h"
#include "virt/manager.h"

/* these two symbols get filled in at link time */
extern u32 __vdev_s;
extern u32 __vdev_cnt;

/* list of queues that MIGHT have pending jobs */
static listHead_s vqueuePendingList;

static virtDev_s *virtDevGetUnsafe(uint n) {
	return ((virtDev_s *const *)(&__vdev_s))[n];
}

uint virtDevCount(void)
{
	return (uint)(&__vdev_cnt);
}

virtDev_s *virtDevGet(uint n)
{
	if (n >= virtDevCount())
		return NULL;
	return virtDevGetUnsafe(n);
}

void virtDevInitAll(void)
{
	/*
	 - initialize virtual IRQ subsystem
	 - initialize global pending vq list
	 - initialize each device
	*/
	virtIrqReset();
	listHeadInit(&vqueuePendingList);
	for (uint i = 0; i < virtDevCount(); i++)
		virtDevInit(virtDevGetUnsafe(i), i);
}

enum VirtRegType {
	DeviceReg = 0,
	ConfigReg = 1,
	QueueReg = 2,
	ManagerReg = 3,
};

static u32 virtManagerRegRead(uint reg) {
	switch(reg) {
		case 0:
			return VIRT_MANAGER_VERSION; // VERSION
		case 1:
			return virtDevCount(); // DEVCOUNT
		case 8: case 9: case 10: case 11:
			return virtIrqGet(reg - 8, VIRQ_VQUEUE); // INTBANK0
		case 12: case 13: case 14: case 15:
			return virtIrqGet(reg - 12, VIRQ_CONFIG); // INTBANK1
		default:
			return 0;
	}
}

static void virtManagerRegWrite(uint reg, u32 val) {
	// ignore all register writes for now
}

static void virtManagerDecodeReg(uint *reg, uint *rtype) {
	*rtype = *reg & 0x03;
	*reg = *reg >> 2;
}

u32 virtDevReadReg(uint dev, uint reg)
{
	uint rtype;
	virtDev_s *vdev;

	virtManagerDecodeReg(&reg, &rtype);

	// manager registers ignore the device field
	if (rtype == ManagerReg)
		return virtManagerRegRead(reg);

	vdev = virtDevGet(dev);
	if (UNLIKELY(!vdev))
		return 0;

	switch(rtype) {
		default: return 0;
		case DeviceReg:
			return virtDevInternalRegRead(vdev, reg);
		case ConfigReg:
			return virtDevCfgRead(vdev, reg);
		case QueueReg:
			return virtDevQueueRegRead(vdev, reg & 0x7F, reg >> 7);
	}
}

void virtDevWriteReg(uint dev, uint reg, u32 val)
{
	uint rtype;
	virtDev_s *vdev;

	virtManagerDecodeReg(&reg, &rtype);

	if (rtype == ManagerReg) {
		virtManagerRegWrite(reg, val);
		return;
	}

	vdev = virtDevGet(dev);
	if (UNLIKELY(!vdev))
		return;

	switch(rtype) {
		case DeviceReg:
			virtDevInternalRegWrite(vdev, reg, val); break;
		case ConfigReg:
			virtDevCfgWrite(vdev, reg, val); break;
		case QueueReg:
			virtDevQueueRegWrite(vdev, reg & 0x7F, reg >> 7, val); break;
	}
}

virtDev_s *virtQueueOwner(virtQueue_s *vq)
{
	return virtDevGetUnsafe(vq->owner);
}

bool virtManagerProcessPending(void)
{
	bool ret = true;
	virtQueue_s *vq;
	listNode_s *nextVq;

	u32 critSection = armEnterCritical();

	do {
		if (listEmpty(&vqueuePendingList)) {
			// no queues left to process
			// let the system sleep a bit
			ret = false;
			break;
		}

		nextVq = listHead(&vqueuePendingList);
		listRemove(nextVq);

		vq = CONTAINER_OF(nextVq, virtQueue_s, node);

		// alert the device that there MIGHT be pending
		// descriptors in this virtqueue
		virtDevProcessQueue(virtQueueOwner(vq), vq);
	} while(0);

	armLeaveCritical(critSection);
	return ret;
}

bool virtManagerAddPending(virtQueue_s *vq)
{
	return listAppendIfNotEmbedded(&vq->node, &vqueuePendingList);
}
