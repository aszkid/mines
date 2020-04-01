#pragma once

#include "hash.h"

struct Position {
	float x, y, z;

	static const uint32_t id() {
		return "POSITIONCOMPONENT"_hash;
	}
};
