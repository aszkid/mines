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

	packed_array_t(packed_array_t&& arr) noexcept
		: elt_sz(arr.elt_sz), max_elts(arr.max_elts), sz(arr.sz)
	{
		data = nullptr;
		dense = nullptr;
		sparse = nullptr;
		std::swap(data, arr.data);
		std::swap(dense, arr.dense);
		std::swap(sparse, arr.sparse);
	}

	~packed_array_t()
	{
		if (data)
			delete[] data;
		if (dense)
			delete[] dense;
		if (sparse)
			delete[] sparse;
	}

	template<typename C>
	static packed_array_t make(size_t max_elts)
	{
		return packed_array_t(sizeof(C), max_elts);
	}

	bool has(entity_t e) const
	{
		return dense[sparse[e.index]] == e;
	}

	uint8_t* get(entity_t e)
	{
		assert(e.index < max_elts);
		assert(dense[sparse[e.index]] == e);
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
		std::printf("[packed_array] removing at e=%zu:%zu\n", e.generation, e.index);
		assert(e.index < max_elts);
		if (dense[sparse[e.index]] != e) {
			std::printf("[packed_array] entity has no component\n");
			return;
		}
		dense[sparse[e.index]] = entity_t::invalid();
		if (sparse[e.index] == --sz) {
			std::printf("[packed_array] at back, rm'd\n");
			return;
		}

		entity_t back = dense[sz];
		dense[sparse[e.index]] = back;
		std::memcpy((void*)&data[sparse[e.index] * elt_sz], (void*)&data[sparse[back.index] * elt_sz], elt_sz);
		sparse[back.index] = sparse[e.index];
		std::printf("[packed_array] switched with back!\n");
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

	void print() {
		std::printf("sparse: [");
		for (size_t i = 0; i < max_elts; i++) {
			std::printf(" %zu->%zu ", i, sparse[i]);
		}
		std::printf("]\ndense:  [");
		for (size_t i = 0; i < max_elts; i++) {
			std::printf(" %zu->", i);
			if (dense[i] == entity_t::invalid()) {
				std::printf("? ");
			} else {
				std::printf("%zu ", dense[i].index);
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
