#pragma once

#include "AssetManager.h"
#include "hash.h"

struct RenderMesh {
	asset_t mesh;
	asset_t material;

	static constexpr uint32_t id() {
		return "RENDERMESH"_hash;
	}
};