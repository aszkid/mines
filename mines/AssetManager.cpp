#include "AssetManager.h"

asset_manager_t::asset_manager_t()
	: map(sizeof(uint8_t*), 64)
{
}

asset_manager_t::~asset_manager_t()
{
}

void asset_manager_t::load(const asset_t asset, uint8_t* data)
{
}

uint8_t* asset_manager_t::get(const asset_t asset)
{
	return nullptr;
}
