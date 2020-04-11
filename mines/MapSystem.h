#pragma once

#include "Entity.h"
#include "AssetManager.h"
#include <cstdint>
#include <hastyNoise.h>
#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>
#include <utility>

struct context_t;

struct chunk_t {
	enum {
		GRASS = 0,
		ROCK,
		WATER,
		_COUNT,
		AIR,
	};
	entity_t entity;
	asset_t meshes[_COUNT];
};

struct map_system_t {

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
	std::unordered_map<glm::ivec3, chunk_t> chunk_cache;
	std::vector<glm::ivec3> chunk_cache_sorted;
};