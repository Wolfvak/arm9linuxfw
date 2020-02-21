#pragma once

#include "common.h"

#include "hw/pxi.h"
#include "virtio/queue.h"

namespace Virtio {
	enum DeviceFeatureBits {
		RING_INDIRECT_DESC = BIT(28),
		RING_EVENT_IDX = BIT(29),
		VERSION_1 = BIT(32),
		ACCESS_PLATFORM = BIT(33),
		RING_PACKED = BIT(34),
		IN_ORDER = BIT(35),
		ORDER_PLATFORM = BIT(36),
	};

	enum DeviceStateBits {
		Acknowledge = BIT(0),
		Driver = BIT(1),
		Driver_OK = BIT(2),
		Features_OK = BIT(3),
		Needs_Reset = BIT(6),
		Failed = BIT(7)
	};

	enum NotificationType {
		QueueChange = 0,
		ConfigChange = BIT(0),
	};

	class Device {
	public:
		Device(u32 dev_id, u32 num_vqs, u64 features);
		u32 InternalID(void) { return this->pxi_id; }
		u32 DevID(void) { return this->device_id; }

		u32 ReadRegister(u32 reg);
		void WriteRegister(u32 reg, u32 val);

		virtual void DoHardwareReset(void) = 0;

		virtual u8 ReadConfig(u32 reg) = 0;
		virtual void WriteConfig(u32 reg, u8 b) = 0;

		virtual int ReadFromReq(const void *addr, u32 reqlen) = 0;
		virtual int WriteToReq(void *addr, u32 reqlen) = 0;

	protected:
		u8 *cfg;

		u32 pxi_id;

		QueueController q_cont;
		u32 device_id, vendor_id, status, config_gen;

		union {
			u64 feat;
			struct { u32 lo, hi; };
		} device_feat, driver_feat;
	};

	class DeviceArray {
	public:
		constexpr DeviceArray(void) : d(nullptr), n(0) {} // uninitialized at the start
		constexpr DeviceArray(Device **d, uint n)
		 : d(d), n(n) {}

		uint Max(void) { return n; }
		Device *operator[](uint i) {
			ASSERT(i < n);
			return d[i];
		}

	private:
		Device **d;
		uint n;
	};

	DeviceArray InstalledDevices(void);
	void NotifyGuest(u32 dev, u32 type);

	void SendNotifications(void);
}
