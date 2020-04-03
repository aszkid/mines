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

	template<typename T>
	T* make(const asset_t asset)
	{
		T* t = (T*)std::malloc(sizeof(T));
		load(asset, (uint8_t*)t);
		return t;
	}

	void load(const asset_t asset, uint8_t *data);
	void release(const asset_t asset);
	uint8_t* allocate_chunk(const asset_t asset, const size_t sz);

	template<typename T>
	inline T* allocate_chunk(const asset_t asset, const size_t count)
	{
		return (T*)allocate_chunk(asset, count * sizeof(T));
	}
	uint8_t* get(const asset_t asset);

	template<typename T>
	inline T* get(const asset_t asset)
	{
		return (T*)get(asset);
	}
private:
	std::unordered_map<asset_t, uint8_t*, asset_hash_f> map;
	std::unordered_map<asset_t, std::vector<uint8_t*>, asset_hash_f> chunks;
};

