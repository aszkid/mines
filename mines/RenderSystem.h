#pragma once

#include <unordered_map>
#include <vector>
#include <tuple>
#include <glad/glad.h>
#include "Entity.h"
#include "PackedArray.h"

struct context_t;
struct IndexedMesh;

//////////////////////////////////////////////
// TODO: consider using the pimpl idiom
//  to abstract the implementation
//////////////////////////////////////////////
struct render_system_t {
	struct cmd_t {
		GLuint vao, vbo, ebo;
		size_t num_indices;
		uint32_t last_update;
		IndexedMesh* mesh;
	};

	render_system_t(context_t* ctx);

	int init();
	void render(entity_t camera);
	void teardown();

	std::unordered_map<entity_t, std::vector<cmd_t>, entity_hash_f> cmds;
	int status;
	context_t* ctx;
	GLuint shader;
};
