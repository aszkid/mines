#pragma once

struct IndexedMesh {
	struct Vertex {
		float x, y, z;
		float nx, ny, nz;
		float r, g, b;
	};
	size_t num_verts;
	size_t num_indices;
	Vertex* vertices;
	unsigned int* indices;
};
