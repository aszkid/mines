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
class render_system_t {
public:
	struct cmd_t {
		GLuint vao, vbo;
	};

	render_system_t(context_t* ctx);
	~render_system_t();

	int init();
	void render();

	packed_array_t cmds;
private:
	int status;
	context_t* ctx;
	GLuint shader;

	void handle_new(entity_t e);
	void handle_update(entity_t e);
	void handle_delete(entity_t e);

};

