#pragma once

#include "hash.h"
#include <glm/vec3.hpp>

struct Camera {
	glm::vec3 pos;
	glm::vec3 up;
	glm::vec3 look;
	float fov;

	static constexpr uint32_t id() {
		return "CAMERA"_hash;
	}
};