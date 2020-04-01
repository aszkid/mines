#pragma once

#include "Entity.h"
#include <vector>
#include <deque>
#include <set>
#include <unordered_map>

class packed_store_t {
public:
	packed_store_t(size_t sz);

	uint8_t* alloc(entity_t e);
	size_t size();
	std::set<entity_t> any();
	bool has(entity_t e);

private:
	std::vector<uint8_t> data;
	std::deque<size_t> free_list;
	std::unordered_map<entity_t, size_t, entity_hash> map;
	size_t sz;
};
