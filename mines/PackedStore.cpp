#include "PackedStore.h"

packed_store_t::packed_store_t(size_t sz)
	: sz(sz)
{
}

uint8_t* packed_store_t::alloc(entity_t e)
{
	auto it = map.find(e);
	if (it != map.end()) {
		return &data[it->second];
	}

	size_t idx = -1;
	if (free_list.size() > 0) {
		idx = free_list.front();
		free_list.pop_front();
	} else {
		idx = data.size();
		data.resize(idx + sz);
	}

	map.insert({ e, idx });
	return &data[idx];
}

size_t packed_store_t::size()
{
	return map.size();
}

std::set<entity_t> packed_store_t::any()
{
	auto s = std::set<entity_t>();
	for (auto& kv : map) {
		s.insert(kv.first);
	}
	return s;
}

bool packed_store_t::has(entity_t e)
{
	return map.find(e) != map.end();
}
