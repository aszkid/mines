#include "Config.h"

int config_t::open(const std::string file)
{
	L = luaL_newstate();
	luaL_openlibs(L);
	if (luaL_dofile(L, file.c_str()))
		return ERROR;
	return OK;
}

config_t::config_t()
	: L(nullptr)
{
}

config_t::~config_t()
{
	if (L)
		lua_close(L);
}
