#pragma once

#include "hash.h"

struct Triangle {
	float vs[9];

	static constexpr uint32_t id() {
		return "TRIANGLE"_hash;
	}
};