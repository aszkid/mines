#pragma once

#include "hash.h"

struct chunk_t {
	uint32_t x, y, z; // unique chunk identifier (x, y, z)
	struct block_t {
		enum {
			AIR, SAND, GRASS, ROCK
		};
		uint8_t type;
	};
	block_t blocks[16][16][16];

	static constexpr uint32_t id() {
		return "CHUNK"_hash;
	}
};