#pragma once

#include <string>
#include <cassert>
#include <lua.hpp>

struct config_t {
	enum { OK, ERROR };

	int open(const std::string file);

	inline void _getglobal(const char* name, int type) {
		if (lua_getglobal(L, name) != type)
			throw;
	}
	inline void _pop() {
		lua_pop(L, 1);
	}

	template<typename T>
	T get(const char* name);

	template<>
	int get(const char *name)
	{
		_getglobal(name, LUA_TNUMBER);
		int val = lua_tointeger(L, -1);
		_pop();
		return val;
	}

	template<>
	float get(const char* name)
	{
		_getglobal(name, LUA_TNUMBER);
		float val = lua_tonumber(L, -1);
		_pop();
		return val;
	}

	template<typename T>
	T get(const char* array, int key);

	template<>
	int get(const char* array, int key)
	{
		_getglobal(array, LUA_TTABLE);
		lua_rawgeti(L, -1, key);
		int val = lua_tointeger(L, -1);
		_pop(); _pop();
		return val;
	}

	template<>
	float get(const char* array, int key)
	{
		_getglobal(array, LUA_TTABLE);
		lua_rawgeti(L, -1, key);
		float val = lua_tonumber(L, -1);
		_pop(); _pop();
		return val;
	}

	config_t();
	~config_t();

	lua_State* L;
};