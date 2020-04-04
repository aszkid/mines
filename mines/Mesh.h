#pragma once

struct Mesh {
	struct Vertex {
		float x, y, z;
		float nx, ny, nz;
	};
	size_t num_verts;
	Vertex *vertices;
};
