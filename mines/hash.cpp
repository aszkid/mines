#include "hash.h"

std::uint32_t hash_str(std::string s)
{
	return detail::fnv1a_32(s.c_str(), s.length());
}
