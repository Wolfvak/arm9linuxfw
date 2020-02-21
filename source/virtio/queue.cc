#include "common.h"

#include "bootalloc.h"
#include "ringbuffer.h"

#include "arm/arm.h"
#include "hw/pxi.h"
#include "virtio/device.h"
#include "virtio/queue.h"

namespace Virtio {
	#define VIRTQ_DESC_F_NEXT	BIT(0)
	#define VIRTQ_DESC_F_WRITE	BIT(1)
	#define VIRTQ_DESC_F_INDIRECT	BIT(2)

	static Ringbuffer<QueueBuffer, 256, u32> globl_jobring;

	typedef struct QueueDescriptor {
		u64 addr;
		u32 len;
		u16 flags;
		u16 next;
	} PACKED QueueDescriptor; static_assert(sizeof(QueueDescriptor) == 16);
	typedef struct QueueAvailRing {
		u16 flags;
		u16 last;
		u16 ring[0];
	} PACKED QueueAvailRing; static_assert(sizeof(QueueAvailRing) == 4);
	typedef struct QueueUsedRing {
		u16 flags;
		u16 last;
		struct PACKED { u32 id; u32 len; } ring[0];
	} PACKED QueueUsedRing; static_assert(sizeof(QueueUsedRing) == 4);

	class Queue {
	public:
		constexpr Queue(QueueController *qc) : qc(qc),
			ready(0), avail_last(0), used_last(0), maxidx_mask(0),
			desc(nullptr), avail(nullptr), used(nullptr) {}

		const QueueDescriptor *GetDescriptor(uint i)
		{ return (const QueueDescriptor*) &this->desc[i]; }

		QueueController *Controller(void) { return this->qc; }

		u32 &Ready(void) { return this->ready; }

		int PopAvailIndex(void)
		{
			u32 index, last_index;

			index = this->avail_last & this->maxidx_mask;
			last_index = this->avail->last & this->maxidx_mask;

			if (index == last_index)
				return -1;

			this->avail_last = index + 1;
			return this->avail->ring[index];
		}

		void PushFinishedIndex(u16 head_idx, u32 written_len)
		{
			u16 idx = this->used_last & this->maxidx_mask;

			CORGI_LOGF("PUSHFINISHEDINDEX %d %d %d\n", idx, head_idx, written_len);

			this->used->ring[idx].id = head_idx;
			this->used->ring[idx].len = written_len;

			// make sure the ring contents are
			// updated before the ring itself is
			ARM::DSB();

			this->used_last = idx + 1;
			this->used->last += 1;
			ARM::DSB(); // synchronization barrier
		}

		void NotifyNewBuffers(void)
		{
			int ring_idx;
			QueueBuffer qbuf;

			qbuf.q = this;
			qbuf.total_written = 0;

			do {
				ASSERT(!globl_jobring.Full()); // should never happen

				ring_idx = this->PopAvailIndex();
				if (ring_idx < 0)
					break; // no further buffers are available

				qbuf.head_idx = qbuf.curr_idx = ring_idx;
				globl_jobring.Store(qbuf);

				CORGI_LOGF("NEW HEAD %d\n", ring_idx);
			} while(1);
		}

		void SetDescriptorAddr(u32 addr) {
			this->desc = (QueueDescriptor*)addr;
		}

		void SetAvailRingAddr(u32 addr) {
			this->avail = (QueueAvailRing*)addr;
		}

		void SetUsedRingAddr(u32 addr) {
			this->used = (QueueUsedRing*)addr;
		}

		u16 &NumCurrentMask(void) {
			return this->maxidx_mask;
		}

	protected:
		QueueController *qc;

		u32 ready;
		u16 avail_last, used_last, maxidx_mask;
		QueueDescriptor *desc;
		QueueAvailRing *avail;
		QueueUsedRing *used;
	};

	void QueueBuffer::Process(void)
	{
		Device *dev;
		const QueueDescriptor *desc;

		dev = this->q->Controller()->Owner();
		desc = this->q->GetDescriptor(this->curr_idx);

		while(1) {
			int state;
			u32 reqlen;
			void *addr;

			CORGI_LOGF("WILL PROCESS JOB FROM DEVICE %d:%d WITH INDEX %d/%d %x ADDR %x FLAGS %d LEN %d\n",
				dev->InternalID(), dev->DevID(), this->head_idx, this->curr_idx, (u32)desc, (u32)(desc->addr), desc->flags, desc->len);

			addr = (void*)desc->addr;
			reqlen = desc->len;

			// indirect descriptors are not supported, abort if they show up
			ASSERT((desc->flags & VIRTQ_DESC_F_INDIRECT) == 0);
			if (desc->flags & VIRTQ_DESC_F_WRITE) {
				state = dev->WriteToReq(addr, reqlen);
				this->total_written += reqlen;
			} else {
				state = dev->ReadFromReq(addr, reqlen);
			}

			CORGI_LOGF("GOT %d STATE BACK\n", state);
			switch(state) {
				case Deferred:
					break;

				case Processing:
					break;

				case Finished:
					break;
			}

			if (desc->flags & VIRTQ_DESC_F_NEXT) {
				CORGI_LOGF("ADVANCING FROM DESCRIPTOR %d TO NEXT %d\n",
					this->curr_idx, desc->next);
				this->curr_idx = desc->next;
				desc = this->q->GetDescriptor(desc->next);
			} else {
				CORGI_LOGF("PUSHING DESCRIPTOR %d AS FINISHED WITH LENGTH %d\n",
					this->head_idx, this->total_written);

				// push to used ring
				this->q->PushFinishedIndex(this->head_idx, this->total_written);

				// notify guest
				NotifyGuest(dev->InternalID(), QueueChange);
				break;
			}
		}
	}

	enum QueueRegisters {
		QueueNumMax = 0x00,
		QueueNumCurrent = 0x01,

		QueueReady = 0x02,
		QueueNotify = 0x03,

		QueueDesc = 0x04,
		QueueAvail = 0x05,
		QueueUsed = 0x06,
	};

	QueueController::QueueController(Device *owner, uint vq_num)
	{
		this->owner = owner;
		this->vq_num = vq_num;
		this->qarr = (Queue*)bootalloc(vq_num * sizeof(Queue));
		for (uint i = 0; i < vq_num; i++)
			this->qarr[i] = Queue(this);
	}

	Queue *QueueController::GetQueue(uint qidx)
	{
		if (LIKELY(qidx < this->vq_num))
			return &this->qarr[qidx];
		return nullptr;
	}

	u32 QueueController::ReadRegister(uint qidx, uint reg)
	{
		Queue *q = this->GetQueue(qidx);
		CORGI_LOGF("READ_QUEUE_REG %d %d\n", qidx, reg);

		if (UNLIKELY(q == nullptr))
			return 0;

		switch(reg) {
			case QueueNumMax:
				return this->vq_num;

			case QueueNumCurrent:
				return q->NumCurrentMask() + 1;

			case QueueReady:
				return q->Ready();

			default:
				CORGI_LOGF("UNKNOWN QUEUE %d REGISTER %d READ\n", qidx, reg);
				return 0;
		}
	}

	void QueueController::WriteRegister(uint qidx, uint reg, u32 val)
	{
		Queue *q = this->GetQueue(qidx);
		CORGI_LOGF("WRITE_QUEUE_REG %d %d %x\n", qidx, reg, val);

		if (UNLIKELY(q == nullptr)) {
			CORGI_LOGF("AVOIDED WRITE TO UNAVAILABLE QUEUE\n");
			return;
		}

		switch(reg) {
			case QueueNumCurrent:
				ASSERT(NUM_IS_POW2(val));
				q->NumCurrentMask() = val-1;
				break;

			case QueueReady:
				q->Ready() = val;
				break;

			case QueueNotify:
				q->NotifyNewBuffers();
				break;

			case QueueDesc:
				q->SetDescriptorAddr(val);
				break;

			case QueueAvail:
				q->SetAvailRingAddr(val);
				break;

			case QueueUsed:
				q->SetUsedRingAddr(val);
				break;

			default:
				CORGI_LOGF("UNKNOWN QUEUE %d REGISTER %d WRITE %x\n", qidx, reg, val);
				break;
		}
	}

	bool NextJob(QueueBuffer &job)
	{
		ARM::CriticalSection critlock;
		if (LIKELY(!globl_jobring.Empty())) {
			globl_jobring.Fetch(job);
			return true;
		} else {
			return false;
		}
	}
}
