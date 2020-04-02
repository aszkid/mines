#pragma once

#include <deque>
#include <vector>
#include <unordered_map>

#include "Entity.h"
#include "PackedArray.h"

struct state_msg_header_t {
	enum Type {
		C_NEW, C_UPDATE, C_DELETE
	};
	Type type;
	entity_t e;
	size_t idx;
};

//////////////////////////////////////////////////////////////
// TODO: keep NEWs at front of cdata and UPDATEs at back
//       or have a vector for each; would give better cache
//       locality when materializing the ss
//////////////////////////////////////////////////////////////
struct state_stream_t {
	state_stream_t(size_t csize)
		: csize(csize)
	{}

	std::vector<state_msg_header_t> events_back;
	std::vector<state_msg_header_t> events;
	std::vector<uint8_t> cdata;
	size_t csize;

	template<typename C>
	void push_new(entity_t e, C&& c)
	{
		const size_t old_sz = cdata.size();
		cdata.resize(old_sz + sizeof(C));
		C* ptr = (C*)&cdata[old_sz];
		*ptr = c;
		events.push_back({ state_msg_header_t::C_NEW, e, old_sz });
	}

	void swap()
	{
		events.swap(events_back);
		events.clear();
		cdata.clear();
	}
};

////////////////////////////////////////////////////////////////////////////////////////
// TODO: right now, accessing a component chases __5 pointers__ (!!!!!)
//       (map -> store ptr -> store -> sparse -> dense -> data)
//       if we avoid unordered_map, keep our stores as a packed_array_t[MAX_COMPONENTS],
//       and merge the dense-data arrays, we could cut down to 2 pointers chased
////////////////////////////////////////////////////////////////////////////////////////
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
		auto it = changelogs.find(cID);
		if (it == changelogs.end()) {
			it = changelogs.emplace(cID, new state_stream_t(sizeof(C))).first;
		}

		state_stream_t* ss = it->second;
		ss->push_new<C>(e, std::move(component));
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
		if (it == stores.end())
			return collection_t<C*>{ 0 };
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

	// materialize state stream for each component
	void materialize()
	{
		for (auto& chlog : changelogs) {
			for (auto& msg : chlog.second->events) {
				switch (msg.type) {
				case state_msg_header_t::C_NEW:
					auto it = stores.find(chlog.first);
					if (it == stores.end()) {
						it = stores.emplace(chlog.first, new packed_array_t(chlog.second->csize, 2048)).first;
					}
					it->second->emplace(msg.e, &chlog.second->cdata[msg.idx]);
					break;
				}
			}

			// swap state streams
			chlog.second->swap();
		}
	}

	state_stream_t* get_state_stream(uint32_t cID)
	{
		auto it = changelogs.find(cID);
		if (it == changelogs.end())
			return nullptr;
		return it->second;
	}

	template<typename C>
	state_stream_t* get_state_stream()
	{
		return get_state_stream(C::id());
	}

private:
	std::vector<entity_t> entities;
	std::deque<entity_t> free_entities;
	int ct;

	std::unordered_map<uint32_t, packed_array_t*> stores;
	std::unordered_map<uint32_t, state_stream_t*> changelogs;
};