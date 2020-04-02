#pragma once

#include <glad/glad.h>
#include <unordered_map>
#include "Entity.h"

struct context_t;

class render_system_t {
public:
	render_system_t(context_t* ctx);
	~render_system_t();

	int init();
	void render();

	std::unordered_map<entity_t, GLuint, entity_hash_f> vaos;

private:
	int status;
	context_t* ctx;
	GLuint shader;
};

