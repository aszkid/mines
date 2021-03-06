#pragma once

#include <cassert>
#include <utility>
#include <cstdio>
#include <cstring>

template<typename E>
class packed_array_t {
public:
	packed_array_t(size_t elt_sz, size_t max_elts)
		: elt_sz(elt_sz), max_elts(max_elts), sz(0)
	{
		data = new uint8_t[max_elts * elt_sz];
		dense = new E[max_elts];
		sparse = new size_t[max_elts];

		for (size_t i = 0; i < max_elts; i++) {
			dense[i] = E::invalid();
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

	bool has(E e) const
	{
		return dense[sparse[e.index()]] == e;
	}

	uint8_t* get(E e)
	{
		assert(e.index() < max_elts);
		assert(dense[sparse[e.index()]] == e);
		return &data[sparse[e.index()] * elt_sz];
	}

	template<typename C>
	C& get(E e)
	{
		assert(sizeof(C) == elt_sz);
		return *(C*)get(e);
	}

	void emplace_batch(E* e, uint8_t* data, size_t ct)
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
			sparse[e[i].index()] = sz++;
			dense[sz] = e[i];
		}
		// copy component data
		std::memcpy(&data[old_sz], data, ct * elt_sz);
	}

	void emplace(E e, uint8_t* bytes)
	{
		assert(e.index() < max_elts);
		if (dense[sparse[e.index()]] != e) {
			assert(sz < max_elts);
			sparse[e.index()] = sz++;
		}

		dense[sparse[e.index()]] = e;
		uint8_t* dest = get(e);
		std::memcpy(dest, bytes, elt_sz);
	}

	template<typename C>
	C& emplace(E e, C& c)
	{
		emplace(e, (uint8_t*)&c);
		return get<C>(e);
	}

	void remove(E e)
	{
		assert(e.index() < max_elts);
		if (dense[sparse[e.index()]] != e)
			return;
		dense[sparse[e.index()]] = E::invalid();
		if (sparse[e.index()] == --sz)
			return;

		E back = dense[sz];
		dense[sparse[e.index()]] = back;
		std::memcpy((void*)&data[sparse[e.index()] * elt_sz], (void*)&data[sparse[back.index()] * elt_sz], elt_sz);
		sparse[back.index()] = sparse[e.index()];
	}

	template<typename C>
	C* any()
	{
		assert(sizeof(C) == elt_sz);
		return (C*)data;
	}

	E* any_entities()
	{
		return dense;
	}

	template<typename C>
	std::pair<E*, C*> any_pair()
	{
		return std::pair<E*, C*>(dense, (C*)data);
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
			if (dense[i] == E::invalid()) {
				std::printf("? ");
			} else {
				std::printf("%zu ", dense[i].index);
			}
		}
		std::printf("]\n");
	}

private:
	uint8_t *data;
	E* dense;
	size_t *sparse;
	size_t sz;
	const size_t elt_sz;
	const size_t max_elts;
};
