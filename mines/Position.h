#pragma once

#include "hash.h"

struct Position {
	float x, y;

	static const uint32_t id() {
		return "POSITION"_hash;
	}
};
