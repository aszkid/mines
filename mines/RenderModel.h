#pragma once

#include "Asset.h"
#include "hash.h"

struct RenderModel {
	enum { MAX_MESHES = 16 };
	asset_t meshes[MAX_MESHES];
	size_t num_meshes;
	uint32_t last_update;
	bool visible;

	static constexpr uint32_t id() {
		return "RenderModel"_hash;
	}
};