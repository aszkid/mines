#pragma once

#include <deque>
#include <vector>
#include <unordered_map>

#include "Entity.h"
#include "PackedArray.h"

class entity_manager_t {
public:
	entity_manager_t();
	~entity_manager_t();

	entity_t new_entity();
	void free_entity(entity_t e);

	template<typename C>
	C* attach(entity_t e, C& component)
	{
		auto it = stores.find(C::id());
		if (it == stores.end()) {
			stores.insert({ e, packed_array_t(sizeof(C), 1024) });
			return attach<C>(e, component);
		}

		packed_array_t& store = it->second;
	}

private:
	std::vector<entity_t> entities;
	std::deque<entity_t> free_entities;
	int ct;

	std::unordered_map<uint32_t, packed_array_t> stores;
};