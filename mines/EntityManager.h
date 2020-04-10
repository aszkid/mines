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
	{
	}

	state_stream_t(state_stream_t &&ss) noexcept
		: csize(ss.csize)
	{
		events.swap(ss.events);
		events_back.swap(ss.events_back);
		cdata.swap(ss.cdata);
	}

	std::vector<state_msg_header_t> events_back;
	std::vector<state_msg_header_t> events;
	std::vector<uint8_t> cdata;
	size_t csize;

	template<typename C>
	void push_insert(entity_t e, C& c, bool update)
	{
		const size_t old_sz = cdata.size();
		cdata.resize(old_sz + sizeof(C));
		C* ptr = (C*)&cdata[old_sz];
		*ptr = c;
		events.push_back({
			update ? state_msg_header_t::C_UPDATE : state_msg_header_t::C_NEW,
			e, old_sz
		});
	}

	void push_delete(entity_t e)
	{
		events.push_back({ state_msg_header_t::C_DELETE, e, (size_t)-1 });
	}

	void swap()
	{
		events.swap(events_back);
		events.clear();
		cdata.clear();
	}
};

////////////////////////////////////////////////////////////////////////////////////////
// TODO: right now, accessing a component chases __4 pointers__
//       (map -> store -> sparse -> dense -> data)
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

	void new_entity(entity_t* es, size_t ct);
	void free_entity(entity_t *es, size_t ct);

	template<typename C>
	void insert_component(entity_t e, const uint32_t cID, C&& component)
	{
		auto ss = get_ss_or_default<C>(cID);
		auto store = get_store_or_default(cID, sizeof(C));
		if (store->has(e)) {
			ss->push_insert<C>(e, component, true);
		} else {
			ss->push_insert<C>(e, component, false);
		}
	}

	template<typename C>
	inline void insert_component(entity_t e, C&& component)
	{
		insert_component<C>(e, C::id(), std::move(component));
	}

	template<typename C>
	C& get_component(entity_t e, const uint32_t cID)
	{
		auto it = stores.find(cID);
		assert(it != stores.end());
		return it->second.get<C>(e);
	}

	template<typename C>
	inline C& get_component(entity_t e)
	{
		return get_component<C>(e, C::id());
	}

	template<typename C>
	void delete_component(entity_t e)
	{
		auto ss = get_ss_or_default<C>(C::id());
		ss->push_delete(e);
	}

	template<typename C>
	bool has_component(entity_t e)
	{
		auto it = stores.find(C::id());
		if (it == stores.end())
			return false;
		return it->second.has(e);
	}

	template<typename C>
	collection_t<C*> any(const uint32_t cID)
	{
		auto it = stores.find(cID);
		if (it == stores.end())
			return collection_t<C*>{ 0 };
		auto pair = it->second->any_pair<C>();
		return { it->second->size(), pair.first, pair.second };
	}

	template<typename C>
	inline collection_t<C*> any()
	{
		return any<C>(C::id());
	}

	std::vector<entity_t> join(const uint32_t aID, const uint32_t bID);

	template<typename A, typename B>
	inline std::vector<entity_t> join()
	{
		return join(A::id(), B::id());
	}

	void materialize();

	state_stream_t* get_state_stream(uint32_t cID);

	template<typename C>
	inline state_stream_t* get_state_stream()
	{
		return get_state_stream(C::id());
	}

	template<typename C>
	void print()
	{
		auto store = get_store_or_default(C::id(), sizeof(C));
		store->print();
	}

private:
	template<typename C>
	state_stream_t* get_ss_or_default(const uint32_t cID)
	{
		auto it = changelogs.find(cID);
		if (it == changelogs.end()) {
			it = changelogs.emplace(cID, state_stream_t(sizeof(C))).first;
		}
		return &it->second;
	}

	packed_array_t<entity_t>* get_store(const uint32_t cID)
	{
		auto it = stores.find(cID);
		if (it == stores.end()) {
			return nullptr;
		}
		return &it->second;
	}

	packed_array_t<entity_t>* get_store_or_default(const uint32_t cID, const size_t elt_sz)
	{
		auto it = stores.find(cID);
		if (it == stores.end()) {
			it = stores.emplace(cID, packed_array_t<entity_t>(elt_sz, 0x4000)).first;
		}
		return &it->second;
	}

	std::vector<entity_t> entities;
	std::deque<entity_t> free_entities;

	std::unordered_map<uint32_t, packed_array_t<entity_t>> stores;
	std::unordered_map<uint32_t, state_stream_t> changelogs;

	state_stream_t empty_state_stream;
};