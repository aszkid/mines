#include "MapSystem.h"
#include "Chunk.h"
#include <random>
#include "Context.h"
#include "Mesh.h"
#include <glm/vec3.hpp>
#include <hastyNoise.h>

map_system_t::map_system_t(context_t* ctx)
	: ctx(ctx), view_distance(0), seed(0)
{
	n_chunks = 2 * view_distance + 1;
	n_chunks = n_chunks * n_chunks * n_chunks;
	chunks = new chunk_t[n_chunks];
}

map_system_t::~map_system_t()
{
	delete[] chunks;
}

void map_system_t::init()
{
	HastyNoise::loadSimd("./");

	chunk_t chunk;
	entity_t ch;
	ctx->emgr.new_entity(&ch, 1);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dis;
	seed = dis(gen);
	std::printf("[map] seed=%u\n", seed);

	// generate asset storage space for grass blocks
	asset_t grass = "GrassMesh"_hash;
	Mesh* grass_mesh = ctx->assets.make<Mesh>(grass);

	std::vector<glm::vec3> grass_blocks;

	/*FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
	const size_t N = 16;
	float *set = noise->GetPerlinSet(0, 0, 0, N, N, N);
	size_t idx = 0;
	for (size_t x = 0; x < N; x++) {
		for (size_t y = 0; y < N; y++) {
			for (size_t z = 0; z < N; z++) {
				if (set[idx] > 0.f) {
					chunk.blocks[x][y][z].type = chunk_t::block_t::GRASS;
					grass_blocks.emplace_back(x, y, z);
				}
				idx++;
			}
		}
	}
	FastNoiseSIMD::FreeNoiseSet(set);
	delete[] noise;*/

	ctx->emgr.insert_component<chunk_t>(ch, std::move(chunk));

	// each block needs 36 vertices
	//grass_mesh->num_verts = 36 * grass_blocks.size();
	//grass_mesh->vertices = ctx->assets.allocate_chunk<Mesh::Vertex>(grass, 36 * grass_mesh->num_verts);
	grass_mesh->num_verts = 3;
	grass_mesh->vertices = ctx->assets.allocate_chunk<Mesh::Vertex>(grass, 3);
	Mesh::Vertex* v = grass_mesh->vertices;
	for (glm::vec3& pos : grass_blocks) {
		// tri 1
		v->x = pos.x; v->y = pos.y; v->z = pos.z; v += 1;
		v->x = pos.x; v->y = pos.y; v->z = pos.z + 1.f; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z;
	}
}

void map_system_t::update()
{
}
