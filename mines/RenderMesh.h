#pragma once

#include "AssetManager.h"
#include "hash.h"
#include <glm/vec3.hpp>

struct RenderMesh {
	asset_t mesh;
	glm::vec3 color;

	static constexpr uint32_t id() {
		return "RENDERMESH"_hash;
	}
};