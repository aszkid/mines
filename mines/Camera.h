#pragma once

#include "hash.h"
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camera {
	glm::vec3 pos;
	glm::vec3 up;
	float pitch, yaw, roll;
	float fov;

	glm::vec3 look() const {
		glm::vec3 look;
		look.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		look.y = sin(glm::radians(pitch));
		look.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		return look;
	}

	static constexpr uint32_t id() {
		return "CAMERA"_hash;
	}
};