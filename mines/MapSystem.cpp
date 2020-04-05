#include "MapSystem.h"
#include "Chunk.h"
#include <random>
#include "Context.h"
#include "IndexedMesh.h"
#include "IndexedRenderMesh.h"
#include <glm/glm.hpp>
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

struct tmp_block_t {
	glm::vec3 pos;
	glm::vec3 color;
};

static void generate_chunk_mesh(asset_manager_t *assets, asset_t a, std::vector<tmp_block_t> &blocks)
{
	IndexedMesh* mesh = assets->make<IndexedMesh>(a);
	mesh->num_verts = 8 * blocks.size();
	mesh->vertices = assets->allocate_chunk<IndexedMesh::Vertex>(a, mesh->num_verts);
	mesh->num_indices = 36 * blocks.size();
	mesh->indices = assets->allocate_chunk<unsigned int>(a, mesh->num_indices);
	IndexedMesh::Vertex* v = mesh->vertices;
	unsigned int* i = mesh->indices;

	static const glm::vec3 normals[8] = {
		// bottom face
		glm::normalize(glm::vec3(-1.f, -1.f, -1.f)),
		glm::normalize(glm::vec3(-1.f, -1.f,  1.f)),
		glm::normalize(glm::vec3( 1.f, -1.f,  1.f)),
		glm::normalize(glm::vec3( 1.f, -1.f, -1.f)),
		// top face
		glm::normalize(glm::vec3(-1.f,  1.f, -1.f)),
		glm::normalize(glm::vec3(-1.f,  1.f,  1.f)),
		glm::normalize(glm::vec3( 1.f,  1.f,  1.f)),
		glm::normalize(glm::vec3( 1.f,  1.f, -1.f))
	};

#define RGB v->r = color.r; v->g = color.g; v->b = color.b
#define COPY_NORMAL(k) std::memcpy(&v->nx, &normals[k], 3 * sizeof(float))

	static const unsigned int indices[36] = {
		0, 1, 2,    0, 2, 3, // bottom
		4, 5, 6,    4, 6, 7, // top
		2, 3, 6,    6, 3, 7, // right
		0, 1, 4,    1, 5, 4, // left
		5, 1, 2,    5, 2, 6, // front
		4, 0, 3,    4, 3, 7  // back
	};
	
	size_t block_n = 0;
	for (auto& blk : blocks) {
		// indexed, we need only 8 vertices and 36 indices
		// (432 bytes vs 1296 bytes un-indexed)
		glm::vec3& pos = blk.pos;
		glm::vec3& color = blk.color;
		// bottom face (CCW)
		v->x = pos.x; v->y = pos.y; v->z = pos.z; COPY_NORMAL(0); RGB; v += 1;
		v->x = pos.x; v->y = pos.y; v->z = pos.z + 1.f; COPY_NORMAL(1); RGB; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z + 1.f; COPY_NORMAL(2); RGB; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z; COPY_NORMAL(3); RGB; v += 1;
		// top face (CCW)
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z; COPY_NORMAL(4); RGB; v += 1;
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z + 1.f; COPY_NORMAL(5); RGB; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z + 1.f; COPY_NORMAL(6); RGB; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z; COPY_NORMAL(7); RGB; v += 1;
		// indices
		for (size_t j = 0; j < 36; j++) {
			*i = 8 * block_n + indices[j];
			i++;
		}
		block_n++;
	}
}

void map_system_t::init()
{
	chunk_t chunk;
	entity_t chunk_entity;
	ctx->emgr.new_entity(&chunk_entity, 1);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dis;
	seed = dis(gen);
	std::printf("[map] seed=%u\n", seed);

	// generate asset storage space for mesh
	asset_t mesh = "BlockMesh"_hash;
	std::vector<tmp_block_t> blocks;

	const size_t N = 60;
	HastyNoise::loadSimd("./");
	size_t fastestSIMD = HastyNoise::GetFastestSIMD();
	auto noise = HastyNoise::CreateNoise(seed, fastestSIMD);
	noise->SetNoiseType(HastyNoise::NoiseType::SimplexFractal);
	noise->SetFrequency(0.05f);
	HastyNoise::FloatBuffer buffer = noise->GetNoiseSet(0, 0, 0, N, N, N);

	size_t idx = 0;
	for (size_t x = 0; x < N; x++) {
		for (size_t y = 0; y < N; y++) {
			for (size_t z = 0; z < N; z++) {
				float val = buffer.get()[idx];
				if (val > 0.f && val < .2f) {
					//chunk.blocks[x][y][z].type = chunk_t::block_t::GRASS;
					blocks.push_back({ glm::vec3(x, y, z), glm::vec3(0.f, 1.f, 0.f) });
				} else if (val > .2f) {
					//chunk.blocks[x][y][z].type = chunk_t::block_t::ROCK;
					blocks.push_back({ glm::vec3(x, y, z), glm::vec3(.4f, .4f, .4f) });
				} else {
					//chunk.blocks[x][y][z].type = chunk_t::block_t::AIR;
				}
				idx++;
			}
		}
	}

	generate_chunk_mesh(&ctx->assets, mesh, blocks);
	ctx->emgr.insert_component<chunk_t>(chunk_entity, std::move(chunk));
	ctx->emgr.insert_component<IndexedRenderMesh>(chunk_entity, { mesh });
}

void map_system_t::update()
{
}
