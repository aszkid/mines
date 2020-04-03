#pragma once

#include <cstdint>
#include <unordered_map>
#include "PackedArray.h"

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

class asset_manager_t
{
public:
	asset_manager_t();
	~asset_manager_t();

	void load(const asset_t asset, uint8_t *data);
	uint8_t* get(const asset_t asset);
private:
	std::unordered_map<asset_t, uint8_t*, asset_hash_f> map;
};

