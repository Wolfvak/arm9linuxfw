#include "common.h"
#include "virtio/device.h"

#define HWRNG_DEVICE_ID	4

class rng_vdev : public Virtio::Device {
public:
	// single virtual queue
	// no extra features
	rng_vdev(void)
	 : Virtio::Device(HWRNG_DEVICE_ID, 1, 0) {}

	// config isn't available, stub them out
	u8 ReadConfig(u32 reg) { return 0xFF; }
	void WriteConfig(u32 reg, u8 b) {}

	// no hw reset is needed
	void DoHardwareReset(void) {}

	// the RNG driver doesnt write data to the host
	// maybe it should report an error if this is called?
	int ReadFromReq(const void *addr, u32 len) {
		return Virtio::Finished;
	}

	int WriteToReq(void *addr, u32 len) {
		u32 *buf = (u32*)addr; // assume the buffer is 4byte aligned
		len /= 4;
		while(len--) // TODO: optimize for large (~>16k) buffers
			*(buf++) = *MMIO_REG(0x10010000, 0x1000, u32);
		return Virtio::Finished;
	}
};

static rng_vdev rng_device;
