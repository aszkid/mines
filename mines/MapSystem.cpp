#include "MapSystem.h"
#include "Position.h"
#include "Camera.h"
#include <random>
#include <sstream>
#include "Context.h"
#include "IndexedMesh.h"
#include "IndexedRenderMesh.h"
#include <glm/glm.hpp>
#include <algorithm>

static const int CHUNK_SIZE = 8 * 3;

#define VEC3_UNPACK(v) v.x, v.y, v.z


map_system_t::map_system_t(context_t* ctx)
	: ctx(ctx), view_distance(4), seed(0), chunk_coord(0), n_chunks(0)
{}

map_system_t::~map_system_t()
{}

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

static void load_chunk(map_system_t* map, glm::ivec3 chunk_coord, map_system_t::chunk_t *chunk)
{
	IndexedMesh* mesh = map->ctx->assets.get<IndexedMesh>(chunk->mesh_asset);
	generate_chunk(map, chunk->mesh_asset, mesh, chunk_coord.x, chunk_coord.y, chunk_coord.z);
	map->ctx->emgr.insert_component<IndexedRenderMesh>(chunk->entity, { chunk->mesh_asset });
	map->ctx->emgr.insert_component<Position>(chunk->entity, {
		(float)CHUNK_SIZE * glm::vec3(chunk_coord)
	});
}

static void unload_chunk(map_system_t* map, glm::ivec3 chunk_coord, map_system_t::chunk_t* chunk)
{

}

static std::vector<glm::ivec3> get_surrounding_chunks(glm::ivec3 at)
{
	return {
		at + glm::ivec3(-1, 0, -1),
		at + glm::ivec3(0, 0, -1),
		at + glm::ivec3(1, 0, -1),
		at + glm::ivec3(-1, 0, 0),
		at,
		at + glm::ivec3(1, 0, 0),
		at + glm::ivec3(-1, 0, 1),
		at + glm::ivec3(0, 0, 1),
		at + glm::ivec3(1, 0, 1)
	};
}

void map_system_t::init(entity_t camera)
{
	seed = generate_seed();
	HastyNoise::loadSimd("./");
	noise = HastyNoise::CreateNoise(seed, HastyNoise::GetFastestSIMD());
	noise->SetNoiseType(HastyNoise::NoiseType::SimplexFractal);
	noise->SetFrequency(0.05f);

	std::printf("[map] seed=%u\n", seed);
	std::printf("[map] view_distance=%zu\n", view_distance);

	Camera& cam = ctx->emgr.get_component<Camera>(camera);
	chunk_coord = get_chunk_pos(cam.pos);

	std::printf("[map] spawned at chunk (%d, %d, %d)\n", VEC3_UNPACK(chunk_coord));

	uint32_t asset_base = "ChunkMesh"_hash;

	auto to_load = get_surrounding_chunks(chunk_coord);
	std::stringstream ss;
	for (auto& chunk : to_load) {
		ss.clear(); ss << "ChunkMesh" << n_chunks++;
		chunk_t ch;
		ch.mesh_asset = hash_str(ss.str());
		ctx->emgr.new_entity(&ch.entity, 1);
		IndexedMesh* mesh = ctx->assets.make<IndexedMesh>(ch.mesh_asset);
		mesh->num_indices = 0;
		mesh->num_verts = 0;
		load_chunk(this, chunk, &ch);
		chunk_cache.insert({ chunk, ch });
		chunk_cache_sorted.push_back(chunk);
	}
}

void map_system_t::update(entity_t camera)
{
	Camera& cam = ctx->emgr.get_component<Camera>(camera);
	glm::ivec3 new_chunk_coord = get_chunk_pos(cam.pos);
	if (chunk_coord == new_chunk_coord)
		return;

	glm::ivec3 diff = new_chunk_coord - chunk_coord;

	std::vector<glm::ivec3> offsets;
	std::vector <glm::ivec3> masks;
	offsets.reserve(2 * view_distance + 1);
	masks.reserve(2 * view_distance + 1);

	if (diff.x == 1) { // moving +x
		offsets.insert(offsets.end(), { glm::ivec3(1, 0, 0) });
		masks.insert(masks.end(), { glm::ivec3(0, 0, 1) });
	}
	if (diff.x == -1) { // moving -x
		offsets.insert(offsets.end(), { glm::ivec3(-1, 0, 0) });
		masks.insert(masks.end(), { glm::ivec3(0, 0, 1) });
	}
	if (diff.z == -1) { // moving -z
		offsets.insert(offsets.end(), { glm::ivec3(0, 0, -1) });
		masks.insert(masks.end(), { glm::ivec3(1, 0, 0) });
	}
	if (diff.z == 1) { // moving +z
		offsets.insert(offsets.end(), { glm::ivec3(0, 0, 1) });
		masks.insert(masks.end(), { glm::ivec3(1, 0, 0) });
	}

#define VEC3_DOT(w) (w).x*(w).x + (w).y*(w).y + (w).z*(w).z

	auto sort_by_distance = [&new_chunk_coord](glm::ivec3& a, glm::ivec3& b) -> bool {
		const glm::ivec3 dist_a = new_chunk_coord - a;
		const glm::ivec3 dist_b = new_chunk_coord - b;
		const int dot_a = VEC3_DOT(dist_a);
		const int dot_b = VEC3_DOT(dist_b);
		return dot_a < dot_b;
	};

	std::sort(chunk_cache_sorted.begin(), chunk_cache_sorted.end(), sort_by_distance);
	assert(chunk_cache_sorted.size() == chunk_cache.size());
	
	std::vector<glm::ivec3> to_load;
	for (size_t i = 0; i < offsets.size(); i++) {
		glm::ivec3& offset = offsets[i];
		glm::ivec3& mask = masks[i];
		for (int j = -(int)view_distance; j <= (int)view_distance; j++) {
			to_load.push_back((int)view_distance * offset + j * mask + new_chunk_coord);
		}
	}

	const size_t view_on_flight = (2 * view_distance + 1) * (2 * view_distance + 1);

	std::stringstream ss;
	std::vector<glm::ivec3> updated_chunks;
	for (size_t i = 0; i < to_load.size(); i++) {
		glm::ivec3 &chunk = to_load[i];
		chunk_t ch;
		std::printf("[map] loading chunk (%d, %d, %d)\n", VEC3_UNPACK(chunk));
		if (chunk_cache.find(chunk) != chunk_cache.end())
			continue;
		// this is the max cache size
		// TODO should depend on the view distance, but how exactly?
		if (chunk_cache_sorted.size() > view_on_flight * view_on_flight) {
			glm::ivec3 target = chunk_cache_sorted.back();
			chunk_cache_sorted.pop_back();
			auto nh = chunk_cache.extract(target);
			assert(!nh.empty());
			ch = nh.mapped();
			nh.key() = chunk;
			chunk_cache.insert(std::move(nh));
		} else {
			ss.clear(); ss << "ChunkMesh" << n_chunks++;
			ch.mesh_asset = hash_str(ss.str());
			ctx->emgr.new_entity(&ch.entity, 1);
			IndexedMesh* mesh = ctx->assets.make<IndexedMesh>(ch.mesh_asset);
			mesh->num_indices = 0;
			mesh->num_verts = 0;
			chunk_cache.insert({ chunk, ch });
		}
		load_chunk(this, chunk, &ch);
		updated_chunks.push_back(chunk);
	}

	// load into chunk cache sorted
	chunk_cache_sorted.insert(chunk_cache_sorted.end(), updated_chunks.begin(), updated_chunks.end());

	chunk_coord = new_chunk_coord;
}
