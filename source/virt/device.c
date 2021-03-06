#include "common.h"

#include "virt/device.h"
#include "virt/queue.h"

#define VIRTDEV_DEFAULT_VENDOR	0x464C4F57 // 'WOLF'

void virtDevInit(virtDev_s *vdev, uint i)
{
	/*
	 - correctly assign the internal ID
	 - for each virtqueue:
	  - initialize virtqueue
	 - reset device registers and hardware
	*/
	vdev->id = i;

	for (uint q = 0; q < vdev->vqn; q++)
		virtQueueInit(&vdev->vqs[q], i, q);

	virtDevReset(vdev);
}

void virtDevReset(virtDev_s *vdev)
{
	/*
	 - reset all state values to their default
	 - for each virtqueue:
	  - reset virtqueue
	 - run hardware reset
	*/

	vdev->status = 0;
	vdev->cfg = 0;

	vdev->driverFeat.dword = 0;

	for (uint i = 0; i < vdev->vqn; i++)
		virtQueueReset(&vdev->vqs[i]);
	virtDevHardReset(vdev);
}

virtQueue_s *virtDevGetQueue(virtDev_s *vdev, uint i)
{
	if (i >= vdev->vqn)
		return NULL;
	return &vdev->vqs[i];
}

enum DeviceRegisters {
	DeviceID = 0x00,
	VendorID = 0x01,

	Status = 0x02,

	DeviceFeaturesLo = 0x03,
	DeviceFeaturesHi = 0x04,
	DriverFeaturesLo = 0x05,
	DriverFeaturesHi = 0x06,

	ConfigGeneration = 0x07,
};

u32 virtDevInternalRegRead(virtDev_s *v, uint reg)
{
	switch(reg) {
		case DeviceID:
			return v->devId;
		case VendorID:
			return VIRTDEV_DEFAULT_VENDOR;
		case Status:
			return v->status;
		case DeviceFeaturesLo:
			return v->deviceFeat.lo;
		case DeviceFeaturesHi:
			return v->deviceFeat.hi;
		case DriverFeaturesLo:
			return v->driverFeat.lo;
		case DriverFeaturesHi:
			return v->driverFeat.hi;
		case ConfigGeneration:
			return v->cfg;
		default:
			return 0;
	}
}

void virtDevInternalRegWrite(virtDev_s *v, uint reg, u32 val)
{
	// certain register writes should also have some side effects
	switch(reg) {
		case Status:
			v->status = val;
			break;
		case DriverFeaturesLo:
			v->driverFeat.lo = val;
			break;
		case DriverFeaturesHi:
			v->driverFeat.hi = val;
			break;
		default:
			break;
	}
}

u32 virtDevQueueRegRead(virtDev_s *v, uint vqn, u32 reg)
{
	virtQueue_s *vq = virtDevGetQueue(v, vqn);
	if (UNLIKELY(!vq))
		return 0;
	return virtQueueRegRead(vq, reg);
}

void virtDevQueueRegWrite(virtDev_s *v, uint vqn, u32 reg, u32 val)
{
	virtQueue_s *vq = virtDevGetQueue(v, vqn);
	if (LIKELY(vq != NULL))
		virtQueueRegWrite(vq, reg, val);
}
