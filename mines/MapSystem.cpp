#include "MapSystem.h"
#include "Chunk.h"
#include "Position.h"
#include <random>
#include "Context.h"
#include "IndexedMesh.h"
#include "IndexedRenderMesh.h"
#include <glm/glm.hpp>

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

static void generate_chunk_mesh(map_system_t* map, IndexedMesh *mesh, asset_t a, std::vector<tmp_block_t> &blocks)
{
	mesh->num_verts = 24 * blocks.size();
	mesh->vertices = map->ctx->assets.allocate_chunk<IndexedMesh::Vertex>(a, mesh->num_verts);
	mesh->num_indices = 36 * blocks.size();
	mesh->indices = map->ctx->assets.allocate_chunk<unsigned int>(a, mesh->num_indices);
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

static void generate_chunk(map_system_t *map, asset_t asset, IndexedMesh *mesh, int xStart, int yStart, int zStart, const size_t N)
{
	std::vector<tmp_block_t> blocks;
	HastyNoise::FloatBuffer buffer = map->noise->GetNoiseSet(xStart, yStart, zStart, N, N, N);

	size_t idx = 0;
	for (size_t x = 0; x < N; x++) {
		for (size_t z = 0; z < N; z++) {
			for (size_t y = 0; y < N; y++) {
				float val = buffer.get()[x*N*N + z*N + y];
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
				//idx++;
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

void map_system_t::init()
{
	seed = generate_seed();
	HastyNoise::loadSimd("./");
	noise = HastyNoise::CreateNoise(seed, HastyNoise::GetFastestSIMD());
	noise->SetNoiseType(HastyNoise::NoiseType::SimplexFractal);
	noise->SetFrequency(0.05f);

	std::printf("[map] seed=%u\n", seed);
	std::printf("[map] have %zu chunks in flight\n", n_chunks);

	//chunk_t chunk;
	entity_t chunk_entities[4];
	asset_t chunk_assets[4] = { "ChunkMesh1"_hash, "ChunkMesh2"_hash, "CM3"_hash, "CM4"_hash };
	ctx->emgr.new_entity(chunk_entities, 4);

	IndexedMesh* mesh1 = ctx->assets.make<IndexedMesh>(chunk_assets[0]);
	IndexedMesh* mesh2 = ctx->assets.make<IndexedMesh>(chunk_assets[1]);
	IndexedMesh* mesh3 = ctx->assets.make<IndexedMesh>(chunk_assets[2]);
	IndexedMesh* mesh4 = ctx->assets.make<IndexedMesh>(chunk_assets[3]);

	generate_chunk(this, chunk_assets[0], mesh1, 0, 0, 0, 32);
	generate_chunk(this, chunk_assets[1], mesh2, 32, 0, 0, 32);
	generate_chunk(this, chunk_assets[2], mesh3, 0, 32, 0, 32);
	generate_chunk(this, chunk_assets[3], mesh4, 32, 32, 0, 32);
	//ctx->emgr.insert_component<chunk_t>(chunk_entity, std::move(chunk));
	ctx->emgr.insert_component<IndexedRenderMesh>(chunk_entities[0], { chunk_assets[0] });
	ctx->emgr.insert_component<IndexedRenderMesh>(chunk_entities[1], { chunk_assets[1] });
	ctx->emgr.insert_component<IndexedRenderMesh>(chunk_entities[2], { chunk_assets[2] });
	ctx->emgr.insert_component<IndexedRenderMesh>(chunk_entities[3], { chunk_assets[3] });
	ctx->emgr.insert_component<Position>(chunk_entities[0], { glm::vec3(-16.f, 0.f, -84.f) });
	ctx->emgr.insert_component<Position>(chunk_entities[1], { glm::vec3(16.f, 0.f, -84.f) });
	ctx->emgr.insert_component<Position>(chunk_entities[2], { glm::vec3(-16.f, 0.f, -52.f) });
	ctx->emgr.insert_component<Position>(chunk_entities[3], { glm::vec3(16.f, 0.f, -52.f) });
}

void map_system_t::update()
{
}
