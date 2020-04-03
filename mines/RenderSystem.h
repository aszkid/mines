#pragma once

#include <glad/glad.h>
#include <unordered_map>
#include "Entity.h"
#include "Triangle.h"

struct context_t;

class render_system_t {
public:
	struct cmd_t {
		GLuint vao, vbo;
	};

	render_system_t(context_t* ctx);
	~render_system_t();

	int init();
	void render();

	// TODO use packed array!!!
	std::unordered_map<entity_t, cmd_t, entity_hash_f> cmds;
private:
	int status;
	context_t* ctx;
	GLuint shader;

	void handle_new(entity_t e);
	void handle_update(entity_t e);
	void handle_delete(entity_t e);

};

