#include "AssetManager.h"

asset_manager_t::asset_manager_t()
{
}

asset_manager_t::~asset_manager_t()
{
	for (auto& pair : map) {
		std::free((void*)pair.second);
		release(pair.first);
	}
}

void asset_manager_t::load(const asset_t asset, uint8_t* data)
{
	map.emplace(asset, data);
	chunks.insert({ asset, {} });
}

void asset_manager_t::release(const asset_t asset)
{
	auto it = chunks.find(asset);
	assert(it != chunks.end());
	for (auto chunk : it->second)
		delete[] chunk;
	it->second.clear();
}

uint8_t* asset_manager_t::allocate_chunk(const asset_t asset, const size_t sz)
{
	auto it = chunks.find(asset);
	assert(it != chunks.end());
	uint8_t *chunk = new uint8_t[sz];
	it->second.push_back(chunk);
	return chunk;
}

uint8_t* asset_manager_t::get(const asset_t asset)
{
	auto it = map.find(asset);
	if (it == map.end())
		return nullptr;
	return it->second;
}
