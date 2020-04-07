#pragma once

#include "AssetManager.h"
#include "hash.h"
#include <glm/vec3.hpp>

struct IndexedRenderMesh {
	asset_t indexed_mesh;
	bool visible;
	uint32_t last_update;

	static constexpr uint32_t id() {
		return "IndexedRenderMesh"_hash;
	}
};