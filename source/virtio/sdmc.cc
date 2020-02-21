#include "common.h"
#include "virtio/device.h"
#include "hw/sdmmc.h"

#define BLOCK_DEVICE_ID	2

class sdmc_vdev : public Virtio::Device {
public:
	struct PACKED { 
		u64 capacity; 
		u32 size_max; 
		u32 seg_max; 
		struct virtio_blk_geometry { 
				u16 cylinders; 
				u8 heads; 
				u8 sectors; 
		} geometry; 
		u32 blk_size; 
		struct virtio_blk_topology { 
				// # of logical blocks per physical block (log2) 
				u8 physical_block_exp; 
				// offset of first aligned logical block 
				u8 alignment_offset; 
				// suggested minimum I/O size in blocks 
				u16 min_io_size; 
				// optimal (suggested maximum) I/O size in blocks 
				u32 opt_io_size; 
		} topology;
		u8 writeback; 
		u8 unused0[3]; 
		u32 max_discard_sectors; 
		u32 max_discard_seg; 
		u32 discard_sector_alignment; 
		u32 max_write_zeroes_sectors; 
		u32 max_write_zeroes_seg; 
		u8 write_zeroes_may_unmap; 
		u8 unused1[3]; 
	} blk_config;

	// single virtqueue, no extra features
	sdmc_vdev(void)
	 : Virtio::Device(BLOCK_DEVICE_ID, 1, 0), sector_off(0) {
		sdmmc_sdcard_init();
		blk_config.capacity = 0;
		blk_config.capacity = sdmmc_sdcard_size();
	}

	u8 ReadConfig(u32 reg) {
		if (reg >= sizeof(blk_config))
			return 0xFF;
		return ((u8*)(&blk_config))[reg];
	}

	void WriteConfig(u32 reg, u8 b) {}
	void DoHardwareReset(void) {}

	int ReadFromReq(const void *addr, u32 len) {
		typedef struct {
			u32 resv;
			u32 type;
			u64 sector;
		} vblk_t;

		vblk_t *req = (vblk_t*)addr;

		CORGI_LOGF("GOT BLOCK REQUEST %d %d %d\n", req->resv, req->type, (u32)(req->sector));

		this->sector_off = req->sector;
		return Virtio::Finished;
	}

	int WriteToReq(void *addr, u32 len) {
		// fill the passed buffer sd sectors
		u8 *a8 = (u8*)addr;

		if (len < 512) {
			*a8 = 0;
		} else {
			sdmmc_sdcard_readsectors(this->sector_off, len>>9, a8);
			(this->sector_off) += (len >> 9);
		}

		return Virtio::Finished;
	}

	u32 sector_off;
};

static sdmc_vdev sd_device;
