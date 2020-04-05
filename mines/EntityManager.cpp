#include "EntityManager.h"

entity_manager_t::entity_manager_t()
	: empty_state_stream(0)
{
}

entity_manager_t::~entity_manager_t()
{
}

void entity_manager_t::new_entity(entity_t* es, size_t ct)
{
	size_t i = 0;
	while (free_entities.size() > 0 && i < ct) {
		es[i] = free_entities.front();
		es[i].generation++;
		free_entities.pop_front();
		entities[es[i].idx] = es[i];
		i++;
	}

	while (i < ct) {
		es[i] = entity_t{ 0, (uint32_t)entities.size() };
		entities.push_back(es[i]);
		i++;
	}
}

void entity_manager_t::free_entity(entity_t* es, size_t ct)
{
	for (size_t i = 0; i < ct; i++) {
		if (es[i] != entities[es[i].idx])
			continue;
		free_entities.push_back(es[i]);
	}
}

state_stream_t* entity_manager_t::get_state_stream(uint32_t cID)
{
	auto it = changelogs.find(cID);
	if (it == changelogs.end()) {
		return &empty_state_stream;
	}
	return &it->second;
}

void entity_manager_t::materialize()
{
	////////////////////////////////////////////////////
	// TODO
	//  sort state messages by header type:
	//  first process insertions, then deletions, and
	//  finally updates
	////////////////////////////////////////////////////
	for (auto& chlog : changelogs) {
		uint32_t cID = chlog.first;
		state_stream_t* ss = &chlog.second;
		packed_array_t<entity_t>* store = get_store_or_default(cID, ss->csize);
		for (auto& msg : ss->events) {
			switch (msg.type) {
			case state_msg_header_t::C_UPDATE:
				assert(store->has(msg.e));
			case state_msg_header_t::C_NEW:
				store->emplace(msg.e, &ss->cdata[msg.idx]);
				break;
			case state_msg_header_t::C_DELETE:
				store->remove(msg.e);
				break;
			}
		}

		// swap state streams
		ss->swap();
	}
}

std::vector<entity_t> entity_manager_t::join(const uint32_t aID, const uint32_t bID)
{
	auto storeA = get_store(aID);
	auto storeB = get_store(bID);
	assert(storeA != nullptr && storeB != nullptr);

	if (storeA->size() > storeB->size()) {
		return join(bID, aID);
	}

	std::vector<entity_t> out;
	auto As = storeA->any_entities();
	for (size_t i = 0; i < storeA->size(); i++) {
		if (storeB->has(As[i]))
			out.push_back(As[i]);
	}

	return out;
}