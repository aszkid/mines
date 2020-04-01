#pragma once

#include <cassert>
#include <utility>
#include "Entity.h"

class packed_array_t {
public:
	packed_array_t(size_t elt_sz, size_t max_elts)
		: elt_sz(elt_sz), max_elts(max_elts), sz(0)
	{
		data = new uint8_t[max_elts * elt_sz];
		dense = new entity_t[max_elts];
		sparse = new size_t[max_elts];

		for (size_t i = 0; i < max_elts; i++) {
			dense[i] = entity_t::invalid();
			sparse[i] = 0;
		}
	}

	template<typename C, size_t MAX_ELTS>
	static packed_array_t make()
	{
		return packed_array_t(sizeof(C), MAX_ELTS);
	}

	~packed_array_t()
	{
		delete[] data;
		delete[] dense;
		delete[] sparse;
	}

	bool has(entity_t e) const
	{
		return dense[sparse[e.index]] == e;
	}

	uint8_t* get(entity_t e)
	{
		return &data[sparse[e.index] * elt_sz];
	}

	template<typename C>
	C& get(entity_t e)
	{
		assert(sizeof(C) == elt_sz);
		return *(C*)get(e);
	}

	template<typename C>
	C& emplace(entity_t e, C&& c)
	{
		if (dense[sparse[e.index]] != e) {
			assert(sz < max_elts);
			sparse[e.index] = sz++;
		}

		std::printf("emplacing entity=%zu at dense idx %zu\n", uint32_t(e), sparse[e.index]);
		dense[sparse[e.index]] = e;
		C* c_dest = (C*)get(e);
		*c_dest = std::move(c);
		return *c_dest;
	}

	void remove(entity_t e)
	{
		dense[sparse[e.index]] = entity_t::invalid();
		if (e.index == sz--)
			return;

		entity_t back = dense[sz];
		dense[sparse[e.index]] = back;
		std::memcpy((void*)&data[sparse[e.index] * elt_sz], (void*)&data[sparse[back.index] * elt_sz], elt_sz);
		sparse[back.index] = sparse[e.index];
	}

	template<typename C>
	std::pair<entity_t*, C*> any()
	{
		return std::pair<entity_t*, C*>(dense, (C*)data);
	}

	size_t size() const
	{
		return sz;
	}

	template<typename C>
	void print() {
		std::printf("sparse: [");
		for (size_t i = 0; i < max_elts; i++) {
			std::printf(" %zu:%zu ", i, sparse[i]);
		}
		std::printf("]\ndense:  [");
		for (size_t i = 0; i < max_elts; i++) {
			std::printf(" %zu:", i);
			if (dense[i] == entity_t::invalid()) {
				std::printf("? ");
			} else {
				std::printf("%zu (%f) ", dense[i].index, get<C>(entity_t{ 0, i }));
			}
		}
		std::printf("]\n");
	}

private:
	uint8_t *data;
	entity_t* dense;
	size_t *sparse;
	size_t sz;
	const size_t elt_sz;
	const size_t max_elts;
};
