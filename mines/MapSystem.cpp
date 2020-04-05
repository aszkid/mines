#include "MapSystem.h"
#include "Position.h"
#include "Camera.h"
#include <random>
#include "Context.h"
#include "IndexedMesh.h"
#include "IndexedRenderMesh.h"
#include <glm/glm.hpp>

static const int CHUNK_SIZE = 8 * 2;

struct chunk_t {
	uint32_t x, y, z;
	asset_t mesh_asset;
	entity_t entity;
};


map_system_t::map_system_t(context_t* ctx)
	: ctx(ctx), view_distance(0), seed(0), old_chunk_coord(0)
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

static void generate_chunk_mesh(map_system_t* map, IndexedMesh *mesh, asset_t a, std::vector<tmp_block_t> &blocks)
{
	size_t old_num_verts = mesh->num_verts;
	mesh->num_verts = 24 * blocks.size();
	mesh->num_indices = 36 * blocks.size();

	// allocate memory if needed
	// !!!!!!!!! TODO remember to free the block if we need to reallocate! !!!!!!!!!!!!
	if (mesh->num_verts > old_num_verts) {
		map->ctx->assets.free_chunk(a, (uint8_t*)mesh->vertices);
		map->ctx->assets.free_chunk(a, (uint8_t*)mesh->indices);
		mesh->vertices = map->ctx->assets.allocate_chunk<IndexedMesh::Vertex>(a, mesh->num_verts);
		mesh->indices = map->ctx->assets.allocate_chunk<unsigned int>(a, mesh->num_indices);
	}

	IndexedMesh::Vertex* v = mesh->vertices;
	unsigned int* i = mesh->indices;

	static const glm::vec3 normals[6] = {
		glm::normalize(glm::vec3(1.f, 0.f, 0.f)),
		glm::normalize(glm::vec3(0.f, 1.f, 0.f)),
		glm::normalize(glm::vec3(0.f, 0.f, 1.f)),
		glm::normalize(glm::vec3(-1.f, 0.f, 0.f)),
		glm::normalize(glm::vec3(0.f, -1.f, 0.f)),
		glm::normalize(glm::vec3(0.f, 0.f, -1.f)),
	};

#define RGB v->r = color.r; v->g = color.g; v->b = color.b
#define COPY_NORMAL(k) std::memcpy(&v->nx, &normals[k], 3 * sizeof(float))
#define NORMAL_X COPY_NORMAL(0)
#define NORMAL_Y COPY_NORMAL(1)
#define NORMAL_Z COPY_NORMAL(2)
#define NORMAL_NEGX COPY_NORMAL(3)
#define NORMAL_NEGY COPY_NORMAL(4)
#define NORMAL_NEGZ COPY_NORMAL(5)
#define VERTEX_0 v->x = pos.x; v->y = pos.y; v->z = pos.z
#define VERTEX_1 v->x = pos.x; v->y = pos.y; v->z = pos.z + 1.f
#define VERTEX_2 v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z + 1.f
#define VERTEX_3 v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z
#define VERTEX_4 v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z
#define VERTEX_5 v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z + 1.f
#define VERTEX_6 v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z + 1.f
#define VERTEX_7 v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z

	static const unsigned int indices[36] = {
		0, 1, 2,     2, 3, 0,		// bottom
		4, 5, 6,     6, 7, 4,		// top
		8, 9, 10,    10, 11, 8,		// left
		12, 13, 14,  14, 15, 12,	// right
		16, 17, 18,  18, 19, 16,	// front
		20, 21, 22,  22, 23, 20		// back
	};
	
	size_t block_n = 0;
	for (auto& blk : blocks) {
		glm::vec3& pos = blk.pos;
		glm::vec3& color = blk.color;
		// bottom face (CCW)
		VERTEX_0; NORMAL_NEGY; RGB; v += 1;		// 0
		VERTEX_1; NORMAL_NEGY; RGB; v += 1;		// 1
		VERTEX_2; NORMAL_NEGY; RGB; v += 1;		// 2
		VERTEX_3; NORMAL_NEGY; RGB; v += 1;		// 3
		// top face (CCW)
		VERTEX_4; NORMAL_Y; RGB; v += 1;		// 4
		VERTEX_5; NORMAL_Y; RGB; v += 1;		// 5
		VERTEX_6; NORMAL_Y; RGB; v += 1;		// 6
		VERTEX_7; NORMAL_Y; RGB; v += 1;		// 7
		// left face (CCW)
		VERTEX_0; NORMAL_NEGX; RGB; v += 1;		// 8
		VERTEX_1; NORMAL_NEGX; RGB; v += 1;		// 9
		VERTEX_5; NORMAL_NEGX; RGB; v += 1;		// 10
		VERTEX_4; NORMAL_NEGX; RGB; v += 1;		// 11
		// right face (CCW)
		VERTEX_3; NORMAL_X; RGB; v += 1;		// 12
		VERTEX_2; NORMAL_X; RGB; v += 1;		// 13
		VERTEX_6; NORMAL_X; RGB; v += 1;		// 14
		VERTEX_7; NORMAL_X; RGB; v += 1;		// 15
		// front face (CCW)
		VERTEX_1; NORMAL_Z; RGB; v += 1;		// 16
		VERTEX_2; NORMAL_Z; RGB; v += 1;		// 17
		VERTEX_6; NORMAL_Z; RGB; v += 1;		// 18
		VERTEX_5; NORMAL_Z; RGB; v += 1;		// 19
		// back face (CCW)
		VERTEX_0; NORMAL_NEGZ; RGB; v += 1;		// 20
		VERTEX_3; NORMAL_NEGZ; RGB; v += 1;		// 21
		VERTEX_7; NORMAL_NEGZ; RGB; v += 1;		// 22
		VERTEX_4; NORMAL_NEGZ; RGB; v += 1;		// 23
		// indices
		for (size_t j = 0; j < 36; j++) {
			*i = 24 * block_n + indices[j];
			i++;
		}
		block_n++;
	}
}

static void generate_chunk(map_system_t *map, asset_t asset, IndexedMesh *mesh, int xStart, int yStart, int zStart)
{
	const int tex_x = CHUNK_SIZE * xStart;
	const int tex_y = CHUNK_SIZE * zStart;
	const int tex_z = CHUNK_SIZE * yStart;
	std::vector<tmp_block_t> blocks;
	HastyNoise::FloatBuffer buffer = map->noise->GetNoiseSet(tex_x, tex_y, tex_z, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);

	for (size_t x = 0; x < CHUNK_SIZE; x++) {
		for (size_t z = 0; z < CHUNK_SIZE; z++) {
			for (size_t y = 0; y < CHUNK_SIZE; y++) {
				float val = buffer.get()[x * CHUNK_SIZE * CHUNK_SIZE + z * CHUNK_SIZE + y];
				if (val > 0.f && val < .2f) {
					//chunk.blocks[x][y][z].type = chunk_t::block_t::GRASS;
					blocks.push_back({ glm::vec3(x, y, z), glm::vec3(0.f, 1.f, 0.f) });
				}
				else if (val > .2f) {
					//chunk.blocks[x][y][z].type = chunk_t::block_t::ROCK;
					blocks.push_back({ glm::vec3(x, y, z), glm::vec3(.4f, .4f, .4f) });
				}
				else {
					//chunk.blocks[x][y][z].type = chunk_t::block_t::AIR;
				}
			}
		}
	}

	generate_chunk_mesh(map, mesh, asset, blocks);
}

static uint32_t generate_seed()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dis;
	return dis(gen);
}

static glm::ivec3 get_chunk_pos(glm::vec3 world_pos)
{
	glm::ivec3 pos;
	pos.x = static_cast<int>(std::floor(world_pos.x / static_cast<float>(CHUNK_SIZE)));
	pos.y = -1;
	pos.z = static_cast<int>(std::floor(world_pos.z / static_cast<float>(CHUNK_SIZE)));
	return pos;
}

static void load_chunk(map_system_t* map, glm::ivec3 chunk_coord, chunk_t *chunk)
{
	IndexedMesh* mesh = map->ctx->assets.get<IndexedMesh>(chunk->mesh_asset);
	generate_chunk(map, chunk->mesh_asset, mesh, chunk_coord.x, chunk_coord.y, chunk_coord.z);
	map->ctx->emgr.insert_component<IndexedRenderMesh>(chunk->entity, { chunk->mesh_asset });
	map->ctx->emgr.insert_component<Position>(chunk->entity, {
		(float)CHUNK_SIZE * glm::vec3(chunk_coord)
	});
}

void map_system_t::init(entity_t camera)
{
	seed = generate_seed();
	HastyNoise::loadSimd("./");
	noise = HastyNoise::CreateNoise(seed, HastyNoise::GetFastestSIMD());
	noise->SetNoiseType(HastyNoise::NoiseType::SimplexFractal);
	noise->SetFrequency(0.05f);

	std::printf("[map] seed=%u\n", seed);
	std::printf("[map] num_chunks=%zum, view_distance=%zu\n", n_chunks, view_distance);

	Camera& cam = ctx->emgr.get_component<Camera>(camera);
	old_chunk_coord = get_chunk_pos(cam.pos);

	chunks[0].mesh_asset = "ChunkMesh1"_hash;
	ctx->emgr.new_entity(&chunks[0].entity, 1);

	IndexedMesh *mesh = ctx->assets.make<IndexedMesh>(chunks[0].mesh_asset);
	mesh->num_indices = 0;
	mesh->num_verts = 0;
	load_chunk(this, old_chunk_coord, &chunks[0]);
}

void map_system_t::update(entity_t camera)
{
	Camera& cam = ctx->emgr.get_component<Camera>(camera);
	glm::ivec3 chunk_coord = get_chunk_pos(cam.pos);
	if (old_chunk_coord != chunk_coord) {
		load_chunk(this, chunk_coord, &chunks[0]);
		old_chunk_coord = chunk_coord;
	}
}
