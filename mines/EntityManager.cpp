#include "EntityManager.h"

entity_manager_t::entity_manager_t()
{
    ct = 0;
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

entity_t entity_manager_t::new_entity()
{
    entity_t e;

    if (free_entities.size() > 0) {
        e = free_entities.front();
        free_entities.pop_front();
        e = entity_t{ e.generation + 1, e.index };
    } else {
        e = entity_t{ 0, entities.size() };
        entities.push_back(e);
    }

    return e;
}

void entity_manager_t::free_entity(entity_t e)
{
    if (e.generation == entities.at(e.index).generation) {
        free_entities.push_back(e);
        entities.at(e.index) = entity_t{ ~(uint32_t)0, ~(uint32_t)0 };
    }
}
