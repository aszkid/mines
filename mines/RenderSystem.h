#pragma once

#include <glad/glad.h>
#include "Entity.h"
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
	struct indexed_cmd_t {
		GLuint vao, vbo, ebo;
		size_t num_verts;
	};

	render_system_t(context_t* ctx);

	int init();
	void render(entity_t camera);
	void teardown();

	packed_array_t<entity_t> cmds;
	packed_array_t<entity_t> indexed_cmds;
	int status;
	context_t* ctx;
	GLuint shader;
};

