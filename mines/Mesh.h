#pragma once

struct Mesh {
	struct Vertex {
		float x, y, z;
		float nx, ny, nz;
		float r, g, b;
	};
	size_t num_verts;
	Vertex *vertices;
};

int load_mesh(asset_manager_t* amgr, asset_t id, const char* path);