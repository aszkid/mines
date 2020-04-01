#pragma once

#include <deque>
#include <vector>
#include <unordered_map>

#include "Entity.h"
#include "PackedArray.h"

class entity_manager_t {
public:
	template<typename T>
	struct collection_t {
		size_t size;
		entity_t *entities;
		T components;
	};

	entity_manager_t();
	~entity_manager_t();

	entity_t new_entity();
	void free_entity(entity_t e);

	template<typename C>
	void attach_component(entity_t e, const uint32_t cID, C&& component)
	{
		auto it = stores.find(cID);
		if (it == stores.end()) {
			stores.emplace(cID, new packed_array_t(sizeof(C), 2048));
			it = stores.find(cID);
		}

		packed_array_t *store = it->second;
		store->emplace(e, std::move(component));
	}

	template<typename C>
	void attach_component(entity_t e, C&& component)
	{
		attach_component<C>(e, C::id(), std::move(component));
	}

	template<typename C>
	C& get_component(entity_t e, const uint32_t cID)
	{
		auto it = stores.find(cID);
		assert(it != stores.end());
		return it->second->get<C>(e);
	}

	template<typename C>
	C& get_component(entity_t e)
	{
		return get_component<C>(e, C::id());
	}

	template<typename C>
	collection_t<C*> any(const uint32_t cID)
	{
		auto it = stores.find(cID);
		assert(it != stores.end());
		auto pair = it->second->any<C>();
		return { it->second->size(), pair.first, pair.second };
	}

	template<typename C>
	collection_t<C*> any()
	{
		return any<C>(C::id());
	}

	std::vector<entity_t> join(const uint32_t aID, const uint32_t bID)
	{
		auto itA = stores.find(aID);
		auto itB = stores.find(bID);
		assert(itA != stores.end() && itB != stores.end());
		
		auto storeA = itA->second;
		auto storeB = itB->second;
		if (storeA->size() > storeB->size()) {
			return join(bID, aID);
		}

		std::vector<entity_t> out;
		auto As = storeA->any();
		for (size_t i = 0; i < storeA->size(); i++) {
			if (storeB->has(As[i]))
				out.push_back(As[i]);
		}

		return out;
	}

	template<typename A, typename B>
	std::vector<entity_t> join()
	{
		return join(A::id(), B::id());
	}

private:
	std::vector<entity_t> entities;
	std::deque<entity_t> free_entities;
	int ct;

	std::unordered_map<uint32_t, packed_array_t*> stores;
};