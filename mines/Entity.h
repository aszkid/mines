#pragma once

#include <cstdint>

struct entity_t {
	uint32_t generation : 8;
	uint32_t index : 24;

	operator uint32_t (void) const
	{
		return (generation << 24) | index;
	}

	inline static entity_t invalid() {
		return { 0xff, 0xffffff };
	}
};

struct entity_hash_f {
	template<typename T>
	size_t operator() (const T& e) const
	{
		return uint32_t(static_cast<entity_t>(e));
	}
};