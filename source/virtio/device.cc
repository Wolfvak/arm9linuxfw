#include "common.h"

#include "arm/arm.h"

#include "virtio/device.h"
#include "virtio/queue.h"

#include "ringbuffer.h"

#include <atomic>

namespace Virtio {
	#define DEV_MAX	16 // software limit, can be as high as 128

	// regmap
	#define CMD_REG_MASK(x)	((x) & 0x0F)

	// queue
	#define CMD_REGQUEUE_REG(x)	(((x) >> 4) & 0x0F) // [7:4]
	#define CMD_REGQUEUE_IDX(x)	(((x) >> 8) & 0xFF) // [15:8]

	// config
	#define CMD_REGCFG_REG(x)	(((x) >> 4) & 0xFFF) // [15:4]

	static constexpr u32 DeviceMagicSig = 0x74726976; // 'virt'

	static u32 DevicePtrCtr = 0;
	static Device *DevicePtrs[DEV_MAX] = { nullptr };

	enum DeviceRegisters {
		MagicValue = 0x00,

		QueueWindow = 0x01,
		ConfigWindow = 0x02,

		DeviceID = 0x03,
		VendorID = 0x04,

		Status = 0x05,

		DeviceFeaturesLo = 0x06,
		DeviceFeaturesHi = 0x07,
		DriverFeaturesLo = 0x08,
		DriverFeaturesHi = 0x09,

		ConfigGeneration = 0x0A,
	};

	Device::Device(u32 dev_id, u32 num_vqs, u64 features)
	 : q_cont(this, num_vqs)
	{
		features |= VERSION_1 | ACCESS_PLATFORM | ORDER_PLATFORM;

		this->device_id = dev_id;
		this->vendor_id = 0x1000;

		this->status = 0;

		this->device_feat.feat = features;
		this->driver_feat.feat = 0;
		this->config_gen = 0;

		ASSERT(DevicePtrCtr < DEV_MAX);
		this->pxi_id = DevicePtrCtr++;
		DevicePtrs[this->pxi_id] = this;
	}

	u32 Device::ReadRegister(u32 reg)
	{
		CORGI_LOGF("READ_DEVICE_REG %d\n", reg);

		switch(CMD_REG_MASK(reg)) {
			case QueueWindow:
				return this->q_cont.ReadRegister(
					CMD_REGQUEUE_IDX(reg), CMD_REGQUEUE_REG(reg)
				);

			case ConfigWindow:
				return this->ReadConfig(CMD_REGCFG_REG(reg));

			case MagicValue:
				return DeviceMagicSig;

			case DeviceID:
				return this->device_id;

			case VendorID:
				return this->vendor_id;

			case Status:
				return this->status;

			case DeviceFeaturesLo:
				return this->device_feat.lo;

			case DeviceFeaturesHi:
				return this->device_feat.hi;

			case DriverFeaturesLo:
				return this->driver_feat.lo;

			case DriverFeaturesHi:
				return this->driver_feat.hi;

			case ConfigGeneration:
				return this->config_gen;

			default:
				CORGI_LOGF("UNHANDLED DEVICE READ %x\n", reg);
				return 0xFFFFFFFF;
		}
	}

	void Device::WriteRegister(u32 reg, u32 val)
	{
		switch(CMD_REG_MASK(reg)) {
			case QueueWindow:
				this->q_cont.WriteRegister(
					CMD_REGQUEUE_IDX(reg), CMD_REGQUEUE_REG(reg), val
				);
				break;

			case ConfigWindow:
				this->WriteConfig(CMD_REGCFG_REG(reg), val);
				break;

			case Status:
				//this->UpdateStatus(val);
				this->status = val;
				break;

			case DriverFeaturesLo:
				this->driver_feat.lo = val;
				break;

			case DriverFeaturesHi:
				this->driver_feat.hi = val;
				break;

			default:
				CORGI_LOGF("UNHANDLED DEVICE WRITE %x %x\n", reg, val);
				break;
		}
	}

	DeviceArray InstalledDevices(void)
	{ return DeviceArray(DevicePtrs, DevicePtrCtr); }

	void NotifyGuest(u32 dev, u32 type)
	{
		u8 port = (type << 7) | dev;
		while(HW::PXI::RecvSync()); // TODO: replace with powersaving

		HW::PXI::SendSync(port);
		HW::PXI::TriggerSync();
	}
}
