#pragma once

typedef struct virtDesc_s virtDesc_s;

typedef struct virtDev_s virtDev_s;
typedef struct virtQueue_s virtQueue_s;

#include "sys/list.h"

#include "virt/job.h"
#include "virt/irq.h"
#include "virt/device.h"
#include "virt/queue.h"

/** Version implemented */
#define VIRT_MANAGER_VERSION	(1)

/** Set an object's private data (must fit in 4 bytes) */
#define OBJ_SETPRIV(v, p)	do{(v)->priv = (void*)(p);}while(0)

/** Retrieve an object's private data, with typecasting */
#define OBJ_GETPRIV(v, t)	((t)((v)->priv))


/**
	Magic macro that declares a VirtIO device and automatically
	adds it to the global device array at compile time

	@name	- name of the virtual device object
	@prv	- private data pointer
	@dclass	- virtual device type id
	@feat	- extra device features
	@hrst	- hardReset function pointer
	@rcfg	- rdCfg function pointer
	@wcfg	- wrCfg function pointer
	@pbuf	- prBuf function pointer
*/
#define DECLARE_VIRTDEV(name, prv, dclass, feat, vn, hrst, rcfg, wcfg, pbuf) \
	virtQueue_s name##_vqs[vn]; \
	virtDev_s name = { \
		.devId = (dclass), \
		.status = 0, \
		.cfg = 0, \
		.deviceFeat.dword = (feat) | VDEV_F_DEFAULT, \
		.driverFeat.dword = 0, \
		.hardReset = (hrst), \
		.rdCfg = (rcfg), \
		.wrCfg = (wcfg), \
		.prQueue = (pbuf), \
		.vqs = name##_vqs, \
		.vqn = (vn), \
		.priv = (prv) \
	}; \
	const u32 SECTION(".vdev_list") name##_entry = (u32)(&name);

/**
	Initialize device array. MUST be done only once on boot
*/
void virtDevInitAll(void);

/**
	Returns the number of devices present in the system
*/
uint virtDevCount(void);

/**
	Get the pointer to a specific virtual device in the system
	if it exists, or NULL otherwise
*/
virtDev_s *virtDevGet(uint n);

/**
	Reads a device register if valid, otherwise returns 0
*/
u32 virtDevReadReg(uint dev, uint reg);

/**
	Writes a device register
*/
void virtDevWriteReg(uint dev, uint reg, u32 val);

/**
	Get the virtual device that owns the virtqueue
*/
virtDev_s *virtQueueOwner(virtQueue_s *vq);

/**
	Process pending VirtBuffers in the system
	Returns false if there are no more buffers left to process
*/
bool virtManagerProcessPending(void);

/**
	Adds a virtQueue to the pending queue list
	Returns false if it was already present
*/
bool virtManagerAddPending(virtQueue_s *vq);

/**
	Notify the host about changes made to the device
*/
static inline void virtDevNotifyHost(virtDev_s *vdev, uint mode) {
	virtIrqSet(virtDevId(vdev), mode);
	virtIrqSync();
}

static inline int virtQueueFetchJobNew(virtQueue_s *vq, virtJob_s *vjob) {
	int first = virtQueueFetchAvailFirst(vq);
	vjob->firstDesc = first;
	vjob->currentDesc = first;
	vjob->totalWritten = 0;
	/* checking whether this is valid is left up to the caller */
	return first;
}

static inline int virtQueueFetchJobNext(virtQueue_s *vq, virtJob_s *vjob) {
	int next = virtQueueFetchAvailNext(vq, vjob->currentDesc);
	vjob->currentDesc = next;
	/* just like before, the caller needs to check it */
	return next;
}

static inline void virtQueueGetJobDesc(virtQueue_s *vq,
									const virtJob_s *vj, virtDesc_s *vd) {
	virtQueueGetDesc(vq, vj->currentDesc, vd);
}

static inline void virtQueuePushJob(virtQueue_s *vq, const virtJob_s *vjob) {
	virtQueuePushUsed(vq, vjob->firstDesc, vjob->totalWritten);
}
