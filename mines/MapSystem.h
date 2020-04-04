#pragma once

#include <cstdint>

struct context_t;
struct chunk_t;

struct map_system_t {
	map_system_t(context_t* ctx);
	~map_system_t();

	void init();
	void update();

	context_t* ctx;
	chunk_t* chunks;
	size_t n_chunks;
	size_t view_distance;
	uint32_t seed;
};