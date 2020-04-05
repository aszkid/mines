#include "AssetManager.h"

asset_manager_t::asset_manager_t()
{
}

asset_manager_t::~asset_manager_t()
{
	for (auto& pair : map) {
		std::free((void*)pair.second.ptr);
		release(pair.first);
	}
}

void asset_manager_t::load(const asset_t asset, uint8_t* data, size_t sz)
{
	map.insert(std::pair<asset_t, sized_ptr_t>(asset, { data, sz }));
	chunks.insert({ asset, {} });
}

void asset_manager_t::release(const asset_t asset)
{
	auto it = chunks.find(asset);
	assert(it != chunks.end());
	for (auto chunk : it->second)
		delete[] chunk.ptr;
	it->second.clear();
}

static const char* units[] = { "bytes", "kB", "MB", "GB" };
static std::pair<float, unsigned int> sz_to_human(float sz)
{
	unsigned int unit = 0;
	while (sz > 1000.f && unit <= 3) {
		sz /= 1000.f;
		unit++;
	}
	return std::pair<float, unsigned int>(sz, unit);
}

void asset_manager_t::print()
{
	size_t sum = 0;
	for (auto& p : map) {
		std::printf("[assets] inspecting %u (%zu byte plain asset)\n", p.first.id, p.second.sz);
		auto it = chunks.find(p.first);
		size_t chunk_ct = 0;
		size_t total_sz = 0;
		if (it != chunks.end()) {
			for (auto& chunk : it->second) {
				chunk_ct++;
				total_sz += chunk.sz;
			}
		}
		auto res = sz_to_human(total_sz);
		std::printf("[assets]    has %zu chunks, total %.2f %s\n", chunk_ct, res.first, units[res.second]);
		sum += total_sz;
	}
	auto res = sz_to_human(sum);
	std::printf("[assets] done! total asset data: %.2f %s\n", res.first, units[res.second]);
}

uint8_t* asset_manager_t::allocate_chunk(const asset_t asset, const size_t sz)
{
	auto it = chunks.find(asset);
	assert(it != chunks.end());
	uint8_t *chunk = new uint8_t[sz];
	it->second.push_back({ chunk, sz });
	return chunk;
}

uint8_t* asset_manager_t::get(const asset_t asset)
{
	auto it = map.find(asset);
	if (it == map.end())
		return nullptr;
	return it->second.ptr;
}
