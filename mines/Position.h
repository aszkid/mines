#pragma once

#include "hash.h"
#include <glm/vec3.hpp>

struct Position {
	glm::vec3 pos;

	static const uint32_t id() {
		return "POSITION"_hash;
	}
};
