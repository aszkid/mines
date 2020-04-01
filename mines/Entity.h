#pragma once

#include <cstdint>

struct entity_t {
	uint32_t generation : 8;
	uint32_t index : 24;

	inline operator uint32_t (void) const
	{
		return (generation << 24) | index;
	}

	inline static entity_t invalid() {
		return { 0xff, 0xffffff };
	}
};

struct entity_hash {
	inline size_t operator() (const entity_t& e) const
	{
		return uint32_t(e);
	}
};