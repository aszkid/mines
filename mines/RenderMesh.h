#pragma once

#include "AssetManager.h"
#include "hash.h"
#include <glm/vec3.hpp>

struct RenderMesh {
	asset_t mesh;

	static constexpr uint32_t id() {
		return "RENDERMESH"_hash;
	}
};