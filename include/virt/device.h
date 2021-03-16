#pragma once

#include "common.h"

#include "sys/list.h"

typedef struct virtDev_s virtDev_s;
typedef struct virtQueue_s virtQueue_s;

/** VirtIO device feature bits */
enum VirtDevFeatureBits {
	/**< This feature enables the used_event and the avail_event fields */
	VDEV_F_RING_EVENT_IDX = BIT(29),

	/**< Indicates compliance with the v1 specification */
	VDEV_F_VERSION_1 = BIT(32),

	/**< Indicates the ARM9 is behind an IOMMU */
	VDEV_F_ACCESS_PLATFORM = BIT(33),

	/**< Indicates support for packed virtqueue layout */
	VDEV_F_RING_PACKED = BIT(34),

	/**< All buffers are used up in the same order as they're provided */
	VDEV_F_IN_ORDER = BIT(35),

	/**< Forces stronger memory and cache barriers */
	VDEV_F_ORDER_PLATFORM = BIT(36),

	/**< Flags that are always set by default */
	VDEV_F_DEFAULT = VDEV_F_VERSION_1 |
		VDEV_F_ACCESS_PLATFORM | VDEV_F_ORDER_PLATFORM,
};

/** VirtIO device status bits */
enum VirtDevStatusBits {
	/**< The guest recognizes the device as a valid VirtIO device */
	VDEV_S_ACK = BIT(0),

	/**< The guest knows how to drive the device */
	VDEV_S_DRIVER = BIT(1),

	/**< The driver is ready */
	VDEV_S_DRIVEROK = BIT(2),

	/**< The guest has completed feature negotiation */
	VDEV_S_FEATURESOK = BIT(3),

	/**< Something happened and the device needs a reset */
	VDEV_S_NEEDRESET = BIT(6),

	/**< The guest has given up on the device */
	VDEV_S_FAILED = BIT(7),
};

/** VirtIO device types */
enum VirtDevType {
	VDEV_T_BLOCK = 2,	/**< Block device */
	VDEV_T_HWRNG = 4,	/**< Entropy source */
	VDEV_T_CRYPTO = 20,	/**< Hardware cryptography module */
};

/**
 Main VirtIO device class.

 Contains several device fields like device type, status, config
 generation, device and driver features, and function pointers
 to operations that reset the device, read/write its configuration, and
 perform buffer processing.

 It also contains a pointer to the virtqueues that correspond to the device,
 and a private device pointer, to use at the coder's discretion.
*/
typedef struct virtDev_s {
	u32 id;	/**< Internal device ID */
	const u16 devId;	/**< Device class */
	u16 status;	/**< Current device status */
	u32 cfg;	/**< Device configuration generation */

	union {
		u64 dword;
		struct { u32 lo, hi; };
	} deviceFeat, driverFeat;	/**< Device and driver feature bits. */

	/**< Perform any needed hardware/software reset */
	void (*hardReset)(virtDev_s*);

	/**< Read a byte from the config space */
	u8 (*rdCfg)(virtDev_s*, uint);

	/**< Write a byte to the config space */
	void (*wrCfg)(virtDev_s*, uint, u8);

	/**< Start processing any pending jobs in this virtqueue */
	void (*prQueue)(virtDev_s*, virtQueue_s*);

	virtQueue_s *const vqs;	/**< Base of virtqueue array */
	const uint vqn;	/**< Number of virtqueues associated to this device */

	void *priv;	/**< Private device data */
} virtDev_s;

/**
	First-time VirtIO initialization. Only called by the manager.
*/
void virtDevInit(virtDev_s *vdev, uint i);

/**
	VirtIO device reset function. Resets all internal
	registers and calls the hardware reset operation.
*/
void virtDevReset(virtDev_s *vdev);

/** Gets the internal ID of a virtual device */
#define virtDevId(v)	((v)->id)

/**
	Returns a specific VirtQueue associated to the device
	if it exists, otherwise returns a NULL pointer.
*/
virtQueue_s *virtDevGetQueue(virtDev_s *vdev, uint i);

/**
	Read an internal device register.
*/
u32 virtDevInternalRegRead(virtDev_s *vdev, uint reg);

/**
	Read an internal device register.
*/
void virtDevInternalRegWrite(virtDev_s *vdev, uint reg, u32 val);

u32 virtDevQueueRegRead(virtDev_s *v, uint vqn, u32 reg);
void virtDevQueueRegWrite(virtDev_s *v, uint vqn, u32 reg, u32 val);

/** VirtDev operation wrappers */
#define virtDevHardReset(v)	((v)->hardReset)(v)
#define virtDevCfgRead(v, off)	((v)->rdCfg)(v, off)
#define virtDevCfgWrite(v, off, data)	((v)->wrCfg)(v, off, data)
#define virtDevProcessQueue(v, vq)	((v)->prQueue)(v, vq)

/**
	VirtIO device stub functions
	These are "compatible" with the interface
	but dont really do anything and barely use
	any code space.
	To use as stubs when a function isn't really necessary.
*/
void virtDevResetStub(virtDev_s *vdev);
u8 virtDevRdCfgStub(virtDev_s *vdev, uint off);
void virtDevWrCfgStub(virtDev_s *vdev, uint off, u8 val);
void virtDevPrQueueStub(virtDev_s *vdev, virtQueue_s *vq);
