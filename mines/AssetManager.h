#pragma once

#include <cstdint>
#include "PackedArray.h"

struct asset_t {
	uint32_t id;

	asset_t() = default;
	asset_t(uint32_t id) : id(id) {}

	inline operator uint32_t (void) const
	{
		return id;
	}

	inline static asset_t invalid() {
		return { ~(uint32_t)0 };
	}

	inline uint32_t index() const {
		return id;
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
	packed_array_t<asset_t> map;
};

