#pragma once

#include <cassert>
#include <utility>
#include "Entity.h"

class packed_array_t {
public:
	// TODO a copy constructor, so that we can use this by-value
	//  in an std::unordered_map and thus chase 2 pointers
	//  instead of 3 on each component lookup
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

	template<typename C>
	static packed_array_t make(size_t max_elts)
	{
		return packed_array_t(sizeof(C), max_elts);
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

	void emplace_batch(entity_t* e, uint8_t* data, size_t ct)
	{
		// TODO
		// assumes entities have not been registered yet!
		// if you emplace_batch ({... e ...}) after having emplaced e,
		// a hole in `data` will be lost: a workaround is to garbage collect
		// every once in a while, looking for data indices that are incorrectly
		// linked up in its corresponding dense-sparse spot

		// link up sparse-dense
		const size_t old_sz = sz;
		for (size_t i = 0; i < ct; i++) {
			sparse[e[i].index] = sz++;
			dense[sz] = e[i];
		}
		// copy component data
		std::memcpy(&data[old_sz], data, ct * elt_sz);
	}

	void emplace(entity_t e, uint8_t* bytes)
	{
		assert(e.index < max_elts);
		if (dense[sparse[e.index]] != e) {
			assert(sz < max_elts);
			sparse[e.index] = sz++;
		}

		dense[sparse[e.index]] = e;
		uint8_t* dest = get(e);
		std::memcpy(dest, bytes, elt_sz);
	}

	template<typename C>
	C& emplace(entity_t e, C& c)
	{
		emplace(e, &c);
		return get<C>(e);
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

	entity_t* any()
	{
		return dense;
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