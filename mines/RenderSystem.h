#pragma once

#include <glad/glad.h>
#include <unordered_map>
#include "Entity.h"
#include "Triangle.h"

struct context_t;

class render_system_t {
	struct cmd_t {
		GLuint vao, vbo;
	};
	int status;
	context_t* ctx;
	GLuint shader;

	cmd_t new_triangle(Triangle* t);
	void update_triangle(entity_t e);
public:
	render_system_t(context_t* ctx);
	~render_system_t();

	int init();
	void render();

	std::unordered_map<entity_t, cmd_t, entity_hash_f> cmds;

};

