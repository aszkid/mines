#pragma once

#include <cstdint>
#include <unordered_map>
#include "PackedArray.h"
#include "Asset.h"

class asset_manager_t
{
public:
	asset_manager_t();
	~asset_manager_t();

	template<typename T>
	T* make(const asset_t asset)
	{
		assert(map.find(asset) == map.end());
		T* t = (T*)std::malloc(sizeof(T));
		load(asset, (uint8_t*)t, sizeof(T));
		return t;
	}

	void release(const asset_t asset);
	uint8_t* allocate_chunk(const asset_t asset, const size_t sz);

	template<typename T>
	inline T* allocate_chunk(const asset_t asset, const size_t count)
	{
		return (T*)allocate_chunk(asset, count * sizeof(T));
	}
	uint8_t* get(const asset_t asset);

	void free_chunk(const asset_t asset, uint8_t* ptr);

	template<typename T>
	inline T* get(const asset_t asset)
	{
		return (T*)get(asset);
	}

	size_t get_chunk_size(const asset_t asset, uint8_t* ptr);

	void print();
private:
	void load(const asset_t asset, uint8_t* data, size_t sz);

	struct sized_ptr_t {
		uint8_t* ptr;
		size_t sz;
	};
	std::unordered_map<asset_t, sized_ptr_t, asset_hash_f> map;
	std::unordered_map<asset_t, std::vector<sized_ptr_t>, asset_hash_f> chunks;
};

