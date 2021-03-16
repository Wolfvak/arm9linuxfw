#include "common.h"

#include "arm/arm.h"
#include "virt/queue.h"
#include "virt/manager.h"

enum QueueRegisters {
	QueueNumMax = 0x00,
	QueueNumCurrent = 0x01,

	QueueReady = 0x02,
	QueueNotify = 0x03,

	QueueDesc = 0x04,
	QueueAvail = 0x05,
	QueueUsed = 0x06,
};

void virtQueueInit(virtQueue_s *vq, uint owner, uint i)
{
	listNodeInit(&vq->node);
	vq->owner = owner;
	vq->id = i;
	virtQueueReset(vq);
}

void virtQueueReset(virtQueue_s *vq)
{
	listRemove(&vq->node);
	vq->ready = 0;
	vq->sizeMask = 0;
	vq->availIdx = 0;
	vq->usedIdx = 0;
	vq->qDesc = 0;
	vq->qAvail = 0;
	vq->qUsed = 0;
}

u32 virtQueueRegRead(virtQueue_s *vq, uint reg)
{
	switch(reg) {
		case QueueNumMax:
			return VIRTQUEUE_MAX_DESC;
		case QueueNumCurrent:
			return vq->sizeMask + 1;
		case QueueReady:
			return vq->ready;
		case QueueDesc:
			return vq->qDesc;
		case QueueAvail:
			return vq->qAvail;
		case QueueUsed:
			return vq->qUsed;
		default: return 0;
	}
}

enum {
	VQ_DESC_F_NEXT = BIT(0),
	VQ_DESC_F_WRITE = BIT(1),
	VQ_DESC_F_INDIRECT = BIT(2),
};

typedef struct vqDesc_s {
	u64 addr; // high 32 bits of address are always ignored
	u32 len;
	u16 flags;
	u16 next;
} PACKED vqDesc_s;

typedef struct vqAvail_s {
	u16 flags;
	u16 last;
	u16 ring[0];
} PACKED vqAvail_s;

typedef struct vqUsed_s {
	u16 flags;
	u16 last;
	struct { u32 id; u32 len; } PACKED ring[0];
} PACKED vqUsed_s;

static void virtQueueSetDescCount(virtQueue_s *vq, u32 val) {
	/* make sure the size is in range and is a power of two */
	if ((val <= VIRTQUEUE_MAX_DESC) &&
		INT_IS_POW2(val)) {
		vq->sizeMask = val - 1;
	} else {
		vq->sizeMask = 0;
	}
}

void virtQueueRegWrite(virtQueue_s *vq, uint reg, u32 val)
{
	switch(reg) {
		case QueueNumCurrent:
			virtQueueSetDescCount(vq, val);
			break;
		case QueueReady:
			if (vq->sizeMask > 0)
				vq->ready = val;
			break;
		case QueueNotify: /* new buffers are present */
			virtManagerAddPending(vq);
			break;
		case QueueDesc:
			vq->qDesc = val;
			break;
		case QueueAvail:
			vq->qAvail = val;
			break;
		case QueueUsed:
			vq->qUsed = val;
			break;
		default: break;
	}
}

int virtQueueFetchAvailFirst(virtQueue_s *vq)
{
	uint index, avail;
	const vqAvail_s *vqA = (const vqAvail_s*)vq->qAvail;

	if (UNLIKELY(!vq->ready))
		return -1;

	index = vq->availIdx & vq->sizeMask;
	avail = vqA->last & vq->sizeMask;

	if (index == avail)
		return -1;

	vq->availIdx = index + 1;
	return vqA->ring[index];
}

int virtQueueFetchAvailNext(virtQueue_s *vq, u16 prev)
{
	const vqDesc_s *vqD = (const vqDesc_s*)vq->qDesc;
	if (vqD[prev].flags & VQ_DESC_F_NEXT)
		return vqD[prev].next;
	return -1;
}

void virtQueuePushUsed(virtQueue_s *vq, u16 first, u32 len)
{
	vqUsed_s *vqU = (vqUsed_s*)vq->qUsed;
	unsigned index = vq->usedIdx & vq->sizeMask;

	vqU->ring[index].id = first;
	vqU->ring[index].len = len;
	armDataSyncBarrier();

	/*
	 * the first data sync barrier ensures that
	 * any buffer data and the 
	 */

	vq->usedIdx = index + 1;
	vqU->last += 1;
	armDataSyncBarrier();
}

void virtQueueGetDesc(virtQueue_s *vq, u16 index, virtDesc_s *desc)
{
	const vqDesc_s *vqd = (const vqDesc_s*)vq->qDesc;

	if (index <= vq->sizeMask) {
		desc->data = (u8*)((u32)vqd[index].addr);
		desc->length = vqd[index].len;
		desc->dir = (vqd[index].flags & VQ_DESC_F_WRITE) ?
			VDEV_TO_HOST : HOST_TO_VDEV;
	} else {
		desc->data = NULL;
	}
}
