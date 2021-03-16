#pragma once

#include "common.h"
#include "sys/list.h"

#include "virt/desc.h"

/** There can only be up to 128 virtqueues per device */
#define VIRTQUEUE_MAX_COUNT	128

/** Each virtqueue can only hold up to 32k items */
#define VIRTQUEUE_MAX_DESC	32768

/** Virtual queue object */
typedef struct virtQueue_s {
	listNode_s node;	/**< node within the pending vqueue list in the parent device */

	u8 owner;	/**< queue owner internal device ID */

	struct {
		u8 id : 7;	/**< queue index within the device */
		u8 ready : 1;	/**< ready state */
	};

	u16 sizeMask;	/**< size mask for all operations (amount of descriptors - 1) */
	u16 availIdx;	/**< last available index */
	u16 usedIdx;	/**< last used index */
	u32 qDesc;	/**< queue descriptor address */
	u32 qAvail;	/**< queue available ring address */
	u32 qUsed;	/**< queue used ring address */
	void *priv;
} virtQueue_s;

void virtQueueInit(virtQueue_s *vq, uint owner, uint i);

void virtQueueReset(virtQueue_s *vq);

u32 virtQueueRegRead(virtQueue_s *vq, uint reg);
void virtQueueRegWrite(virtQueue_s *vq, uint reg, u32 val);

int virtQueueFetchAvailFirst(virtQueue_s *vq);
int virtQueueFetchAvailNext(virtQueue_s *vq, u16 prev);
void virtQueuePushUsed(virtQueue_s *vq, u16 first, u32 totalWritten);

void virtQueueGetDesc(virtQueue_s *vq, u16 index, virtDesc_s *desc);
