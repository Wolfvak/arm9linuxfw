#include "common.h"

#include "arm/arm.h"

#include "hw/irqc.h"
#include "hw/pxi.h"
#include "hw/sdmmc.h"

#include "virtio/device.h"
#include "virtio/queue.h"

#define PXI_MSG_DEV(m)	(((m) >> 25) & 0x7F)
#define PXI_MSG_CMD(m)	(((m) >> 24) & 1)
#define PXI_MSG_REG(m)	(((m) & (BIT(24) - 1)))

static Virtio::DeviceArray devarr;

static void On_RXNotEmpty(u32 irq)
{
	while(1) {
		Virtio::Device *selected_dev;
		u32 msg, dev, cmd, data;

		if (HW::PXI::RecvEmpty())
			break;

		msg = HW::PXI::Recv();

		dev = PXI_MSG_DEV(msg);
		cmd = PXI_MSG_CMD(msg);
		data = PXI_MSG_REG(msg);

		selected_dev = (dev < devarr.Max()) ? devarr[dev] : nullptr;

		switch(cmd) {
			case 0:
			{ // CMD0: Read from register
				u32 rdreg = selected_dev ? selected_dev->ReadRegister(data) : 0xDEADDEAD;
				HW::PXI::Send(rdreg);
				CORGI_LOGF("READ_REGISTER %x %x\n", data, rdreg);
				break;
			}

			case 1:
			{ // CMD1: Write to register
				u32 wrreg = HW::PXI::Recv();
				if (selected_dev) selected_dev->WriteRegister(data, wrreg);
				CORGI_LOGF("WRITE_REGISTER %x %x\n", data, wrreg);
				break;
			}

			default:
				break;
		}
	}
}

extern "C" void MainLoop(void);
void NORETURN MainLoop(void)
{
	// Reset main hardware blocks
	HW::IRQC::Reset();
	HW::PXI::Reset();
	sdmmc_sdcard_init();

	// Set up virtio-state interrupt callbacks
	HW::IRQC::Enable(HW::PXI::IRQ_SYNC_TRIGGERED, nullptr);
	HW::IRQC::Enable(HW::PXI::IRQ_TX_NOT_FULL, nullptr);
	HW::IRQC::Enable(HW::PXI::IRQ_RX_NOT_EMPTY, On_RXNotEmpty);

	devarr = Virtio::InstalledDevices();

	CORGI_LOG("started arm9-virtio\n");

	// Enable interrupts, wait for new commands
	ARM::EnableInterrupts();
	while(1) {
		Virtio::QueueBuffer buf;
		while(Virtio::NextJob(buf))
			buf.Process();
		ARM::WaitForInterrupt();
	}
}
