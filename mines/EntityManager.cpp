#include "EntityManager.h"

entity_manager_t::entity_manager_t()
{
}

entity_manager_t::~entity_manager_t()
{
    for (auto p : stores) {
        delete p.second;
    }
    for (auto s : changelogs) {
        delete s.second;
    }
}

void entity_manager_t::new_entity(entity_t* es, size_t ct)
{
	size_t i = 0;
	while (free_entities.size() > 0 && i < ct) {
		es[i] = free_entities.front();
		es[i].generation++;
		free_entities.pop_front();
		entities[es[i].index] = es[i];
		i++;
	}

	while (i < ct) {
		es[i] = entity_t{ 0, entities.size() };
		entities.push_back(es[i]);
		i++;
	}
}

void entity_manager_t::free_entity(entity_t* es, size_t ct)
{
	for (size_t i = 0; i < ct; i++) {
		if (es[i] != entities[es[i].index])
			continue;
		free_entities.push_back(es[i]);
	}
}

state_stream_t* entity_manager_t::get_state_stream(uint32_t cID)
{
	auto it = changelogs.find(cID);
	if (it == changelogs.end())
		return nullptr;
	return it->second;
}

void entity_manager_t::materialize()
{
	for (auto& chlog : changelogs) {
		auto it = stores.find(chlog.first);
		if (it == stores.end()) {
			it = stores.emplace(chlog.first, new packed_array_t(chlog.second->csize, 2048)).first;
		}
		for (auto& msg : chlog.second->events) {
			switch (msg.type) {
			case state_msg_header_t::C_INSERT:
			case state_msg_header_t::C_UPDATE:
				it->second->emplace(msg.e, &chlog.second->cdata[msg.idx]);
				break;
			}
		}

		// swap state streams
		chlog.second->swap();
	}
}

std::vector<entity_t> entity_manager_t::join(const uint32_t aID, const uint32_t bID)
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