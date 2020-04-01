#pragma once

#include "hash.h"

struct Velocity {
	float vx;

	static const uint32_t id() {
		return "VELOCITY"_hash;
	}
};
