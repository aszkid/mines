#pragma once

#include "hash.h"

struct Velocity {
	float vx, vy, vz;

	static const uint32_t id() {
		return "VELOCITYCOMPONENT"_hash;
	}
};
