#pragma once

#include <assimp/scene.h>

struct Mesh {
	size_t num_verts;
	aiVector3D* vertices;
	aiVector3D* normals;
};
