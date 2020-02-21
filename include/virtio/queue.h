#pragma once

#include "common.h"

namespace Virtio {
	static constexpr u32 MaxQueues = 4; // low limits in place
	static constexpr u32 MaxQueueDescriptors = 8; // should be enough anyway

	class Queue;
	class Device;
	struct QueueDescriptor; // fwd decls

	enum QueueBufferState {
		Deferred = 0, // Device cant process the request yet
		Processing, // Will be processed asynchronously and sent later on its own
		Finished, // Notify the guest about the completion
	};

	typedef struct {
		Queue *q; // parent queue

		u16 head_idx; // head buffer ring index, remains constant
		u16 curr_idx; // current buffer ring index

		u32 total_written;

		void Process(void);
	} QueueBuffer;

	class QueueController {
	public:
		QueueController(Device *owner, uint vq_num);
		Device *Owner(void) { return this->owner; }

		void Reset(void);

		u32 ReadRegister(uint qidx, uint reg);
		void WriteRegister(uint qidx, uint reg, u32 val);

		Queue *GetQueue(uint qidx);

	protected:
		Device *owner;
		uint vq_num;
		Queue *qarr;
	};

	bool NextJob(QueueBuffer &job);
}
