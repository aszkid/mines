#pragma once

#include "Entity.h"
#include <cstdint>
#include <hastyNoise.h>
#include <glm/vec3.hpp>

struct context_t;
struct chunk_t;

struct map_system_t {
	map_system_t(context_t* ctx);
	~map_system_t();

	void init(entity_t camera);
	void update(entity_t camera);

	context_t* ctx;
	chunk_t* chunks;
	size_t n_chunks;
	size_t view_distance;
	uint32_t seed;
	std::unique_ptr<HastyNoise::NoiseSIMD> noise;

	glm::ivec3 old_chunk_coord;
};