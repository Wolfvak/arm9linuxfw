#pragma once

#include "common.h"

template<typename T, size_t N, typename C = u32>
class Ringbuffer {
public:
	constexpr Ringbuffer(void) : fetch(0), store(0) {}

	constexpr C Capacity(void) {
		return N;
	}

	bool Full(void) {
		return ((this->store + 1) % N) == this->fetch;
	}

	bool Empty(void) {
		return (this->store == this->fetch);
	}

	void Fetch(T &t) {
		t = this->ring[this->fetch];
		this->fetch = (this->fetch + 1) % N;
	}

	void Store(T &t) {
		this->ring[this->store] = t;
		this->store = (this->store + 1) % N;
	}

private:
	C fetch, store;
	T ring[N];
};

