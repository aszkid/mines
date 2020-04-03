#pragma once

#include <glad/glad.h>
#include "Entity.h"
#include "Triangle.h"
#include "PackedArray.h"

struct context_t;

//////////////////////////////////////////////
// TODO: consider using the pimpl idiom
//  to abstract the implementation
//////////////////////////////////////////////
struct render_system_t {
	struct cmd_t {
		GLuint vao, vbo;
		size_t num_verts;
	};

	render_system_t(context_t* ctx);
	~render_system_t();

	int init();
	void render();

	packed_array_t<entity_t> cmds;
	int status;
	context_t* ctx;
	GLuint shader;
};

