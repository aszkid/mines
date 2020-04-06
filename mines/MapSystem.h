#pragma once

#include "Entity.h"
#include "AssetManager.h"
#include <cstdint>
#include <hastyNoise.h>
#include <unordered_map>
#include <glm/vec3.hpp>

struct context_t;

struct ivec3_hash_f {
	template<typename T>
	size_t operator() (const T& e) const
	{
		glm::ivec3 v = static_cast<glm::ivec3>(e);
		uint64_t hx = v.x;
		hx <<= 40;
		uint64_t hy = v.y & 0xffff;
		uint64_t hz = v.z & 0xffffff;
		hz <<= 16;
		return hx ^ hz ^ hy;
	}
};

struct map_system_t {
	struct chunk_t {
		asset_t mesh_asset;
		entity_t entity;
	};

	map_system_t(context_t* ctx);
	~map_system_t();

	void init(entity_t camera);
	void update(entity_t camera);

	context_t* ctx;
	size_t n_chunks;
	size_t view_distance;
	uint32_t seed;
	std::unique_ptr<HastyNoise::NoiseSIMD> noise;

	glm::ivec3 chunk_coord;
	std::unordered_map<glm::ivec3, chunk_t, ivec3_hash_f> chunk_cache;
	std::vector<glm::ivec3> chunk_cache_sorted;
};