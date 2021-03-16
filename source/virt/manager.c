#include "common.h"

#include "arm/arm.h"
#include "hw/pxi.h"

#include "virt/irq.h"
#include "virt/manager.h"

/* these two symbols get filled in at link time */
extern u32 __vdev_s;
extern u32 __vdev_cnt;

/* list of devices that MIGHT have pending jobs */
static listHead_s vdevPendingList;

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
	 - initialize global pending device list
	 - initialize each device
	*/
	virtIrqReset();
	listHeadInit(&vdevPendingList);
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
	bool enqueueDev = false;

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
			enqueueDev = virtDevInternalRegWrite(vdev, reg, val); break;
		case ConfigReg:
			enqueueDev = virtDevCfgWrite(vdev, reg, val); break;
		case QueueReg:
			enqueueDev = virtDevQueueRegWrite(vdev, reg & 0x7F, reg >> 7, val); break;
	}

	if (enqueueDev) {
		/* add the device to the processing queue if needed */
		listAppendIfNotEmbedded(&vdev->node, &vdevPendingList);
	}
}

virtDev_s *virtQueueOwner(virtQueue_s *vq)
{
	return virtDevGetUnsafe(vq->owner);
}

bool virtDevProcessPending(void)
{
	bool ret = true;
	virtDev_s *vdev;
	listNode_s *nextDev, *nextVq;

	u32 critSection = armEnterCritical();

	do {
		if (listEmpty(&vdevPendingList)) {
			// no devices left to process
			// let the system sleep a bit
			ret = false;
			break;
		}

		nextDev = listHead(&vdevPendingList);
		vdev = CONTAINER_OF(nextDev, virtDev_s, node);

		if (listEmpty(&vdev->qlist)) {
			// no queues left to process in this device
			// remove the device from the list and return
			listRemove(nextDev);
			break;
		}

		// take the queue of the pending vq list
		nextVq = listNext(&vdev->qlist);
		listRemove(nextVq);

		// alert the device that there MIGHT be pending
		// descriptors in this virtqueue
		virtDevProcessQueue(vdev, CONTAINER_OF(nextVq, virtQueue_s, node));
	} while(0);

	armLeaveCritical(critSection);
	return ret;
}

/*
	nextVq = listNext(&vdev->qlist);
	vq = CONTAINER_OF(nextVq, virtQueue_s, node);

	firstBufIndex = currentBufIndex = virtQueueFetchFirst(vq);
	if (firstBufIndex < 0) {
		// no more vqueue buffers are present, take the vqueue off the list
		listRemove(nextVq);
		armLeaveCritical(critSection);
		return true;
	}

	writeLen = 0;
	do { // process all the virtbuffers corresponding to a single descriptor
		virtBuf_s vbuf = virtQueueGetDesc(vq, currentBufIndex);
		if (UNLIKELY(!virtBufValid(&vbuf))) {
			// bad index, break out and zero writeLen
			writeLen = 0;
			break;
		}

		writeLen += virtDevProcessBuffer(vdev, vq, vbuf);
		currentBufIndex = virtQueueFetchNext(vq, currentBufIndex);
	} while(currentBufIndex >= 0);

	virtQueuePushUsed(vq, firstBufIndex, writeLen);

	// notify the driver that something happened
	virtIrqSet(virtDevId(vdev), VIRQ_VQUEUE);

	armLeaveCritical(critSection);
	return true;
}
*/
