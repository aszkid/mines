#pragma once

#include <cstdint>

struct asset_t {
	uint32_t id;

	asset_t() = default;
	asset_t(uint32_t id) : id(id) {}

	inline operator uint32_t (void) const
	{
		return id;
	}
};

struct asset_hash_f {
	template<typename T>
	size_t operator() (const T& a) const
	{
		return uint32_t(static_cast<asset_t>(a));
	}
};
