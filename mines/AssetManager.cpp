#include "AssetManager.h"

asset_manager_t::asset_manager_t()
{
}

asset_manager_t::~asset_manager_t()
{
}

void asset_manager_t::load(const asset_t asset, uint8_t* data)
{
	map.emplace(asset, data);
}

uint8_t* asset_manager_t::get(const asset_t asset)
{
	auto it = map.find(asset);
	if (it == map.end())
		return nullptr;
	return it->second;
}
