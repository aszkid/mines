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
#include <array>
#include <omp.h>

static const int CHUNK_SIZE = 32;

#define VEC3_UNPACK(v) v.x, v.y, v.z

map_system_t::map_system_t(context_t* ctx)
	: ctx(ctx), view_distance(3), seed(0), chunk_coord(0), n_chunks(0)
{}

map_system_t::~map_system_t()
{}

struct tmp_block_t {
	glm::vec3 pos;
	glm::vec3 color;
};

////////////////////////////////////////////
// CUBE VERTEX DATA
////////////////////////////////////////////
static const glm::vec3 normals[6] = {
	glm::normalize(glm::vec3(1.f, 0.f, 0.f)),
	glm::normalize(glm::vec3(0.f, 1.f, 0.f)),
	glm::normalize(glm::vec3(0.f, 0.f, 1.f)),
	glm::normalize(glm::vec3(-1.f, 0.f, 0.f)),
	glm::normalize(glm::vec3(0.f, -1.f, 0.f)),
	glm::normalize(glm::vec3(0.f, 0.f, -1.f)),
};
static const glm::vec3 vertices[8] = {
	glm::vec3(0.f),
	glm::vec3(0.f, 0.f, 1.f),
	glm::vec3(1.f, 0.f, 1.f),
	glm::vec3(1.f, 0.f, 0.f),
	glm::vec3(0.f, 1.f, 0.f),
	glm::vec3(0.f, 1.f, 1.f),
	glm::vec3(1.f),
	glm::vec3(1.f, 1.f, 0.f)
};
static const glm::vec3 vertex_data[24] = {
	vertices[0], vertices[1], vertices[2], vertices[3],
	vertices[4], vertices[5], vertices[6], vertices[7],
	vertices[0], vertices[1], vertices[5], vertices[4],
	vertices[3], vertices[2], vertices[6], vertices[7],
	vertices[1], vertices[2], vertices[6], vertices[5],
	vertices[0], vertices[3], vertices[7], vertices[4]
};
static const glm::vec3 normal_data[24] = {
	normals[4], normals[4], normals[4], normals[4],
	normals[1], normals[1], normals[1], normals[1],
	normals[3], normals[3], normals[3], normals[3],
	normals[0], normals[0], normals[0], normals[0],
	normals[2], normals[2], normals[2], normals[2],
	normals[5], normals[5], normals[5], normals[5]
};
static const unsigned int indices[36] = {
	0, 1, 2,     2, 3, 0,		// bottom
	4, 5, 6,     6, 7, 4,		// top
	8, 9, 10,    10, 11, 8,		// left
	12, 13, 14,  14, 15, 12,	// right
	16, 17, 18,  18, 19, 16,	// front
	20, 21, 22,  22, 23, 20		// back
};
static std::array<glm::vec3, 72> gen_packed_cube()
{
	std::array<glm::vec3, 72> arr{ glm::vec3(0) };
	for (size_t i = 0; i < 24; i++) {
		arr[3 * i] = vertex_data[i];
		arr[3 * i + 1] = normal_data[i];
		arr[3 * i + 2] = glm::vec3(0);
	}
	return arr;
}
static std::array <glm::vec3, 72> packed_vertex_data;
////////////////////////////////////////////

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

	for (size_t k = 0; k < blocks.size(); k++) {
		tmp_block_t& blk = blocks[k];
		glm::vec3& pos = blk.pos;
		glm::vec3& color = blk.color;

		// vertex data
		std::memcpy(&mesh->vertices[24 * k], &packed_vertex_data[0], 72 * sizeof(glm::vec3));
		for (size_t j = 0; j < 24; j++) {
			mesh->vertices[24 * k + j].x += pos.x; mesh->vertices[24 * k + j].y += pos.y; mesh->vertices[24 * k + j].z += pos.z;
			mesh->vertices[24 * k + j].r = color.r; mesh->vertices[24 * k + j].g = color.g; mesh->vertices[24 * k + j].b = color.b;
		}

		// indices
		std::memcpy(&mesh->indices[36 * k], indices, 36 * sizeof(unsigned int));
		for (size_t j = 0; j < 36; j++) {
			mesh->indices[36 * k + j] += 24 * k;
		}
	}
}

#define TIME_ENABLE
#ifdef TIME_ENABLE
#define TIME_BEGIN before = SDL_GetTicks()
#define TIME_END(s) delta = SDL_GetTicks() - before; std::printf("[map] (tid=%d) " ## s ## " took %u ms\n", omp_get_thread_num(), delta)
#else
#define TIME_BEGIN {}
#define TIME_END {}
#endif

static void generate_chunk(map_system_t *map, asset_t asset, IndexedMesh *mesh, const glm::uvec3 &tex_offset, const glm::uvec3 &tex_sz, const float *buffer)
{
	uint32_t before, delta;
	std::vector<tmp_block_t> blocks;
	blocks.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);

	for (size_t x = 0; x < CHUNK_SIZE; x++) {
		for (size_t z = 0; z < CHUNK_SIZE; z++) {
			for (size_t y = 0; y < CHUNK_SIZE; y++) {
				const size_t tex_idx = (x + tex_offset.x) * tex_sz.z * tex_sz.y + (z + tex_offset.z) * tex_sz.y + y + tex_offset.y;
				const float val = buffer[tex_idx];
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

static void load_chunks(map_system_t* map, std::vector<glm::ivec3>& to_load)
{
	const size_t view_on_flight = (2 * map->view_distance + 1) * (2 * map->view_distance + 1);

	int max_x = INT_MIN;
	int max_y = INT_MIN;
	int max_z = INT_MIN;
	int min_x = INT_MAX;
	int min_y = INT_MAX;
	int min_z = INT_MAX;

	uint32_t before, delta;

	// load chunk coordinates, ignore hot chunks
	TIME_BEGIN;
	std::vector<std::pair<glm::ivec3, map_system_t::chunk_t>> for_real;
	for (int i = 0; i < to_load.size(); i++) {
		glm::ivec3& chunk = to_load[i];
		map_system_t::chunk_t ch;
		if (map->chunk_cache.find(chunk) != map->chunk_cache.end())
			continue;
		// this is the max cache size we're willing to deal with
		// TODO should depend on the view distance, but how exactly?????
		if (map->chunk_cache_sorted.size() > view_on_flight * view_on_flight) {
			glm::ivec3 target = map->chunk_cache_sorted.back();
			map->chunk_cache_sorted.pop_back();
			auto nh = map->chunk_cache.extract(target);
			ch = nh.mapped();
			nh.key() = chunk;
			map->chunk_cache.insert(std::move(nh));
		}
		else {
			static uint32_t base = "ChunkMesh"_hash;
			ch.mesh_asset = base++;
			map->ctx->emgr.new_entity(&ch.entity, 1);
			IndexedMesh* mesh = map->ctx->assets.make<IndexedMesh>(ch.mesh_asset);
			mesh->num_indices = 0;
			mesh->num_verts = 0;
			map->chunk_cache.insert({ chunk, ch });
		}
		for_real.push_back({ chunk, ch });
		map->chunk_cache_sorted.push_back(chunk);

		max_x = std::max(chunk.x, max_x);
		max_y = std::max(chunk.y, max_y);
		max_z = std::max(chunk.z, max_z);
		min_x = std::min(chunk.x, min_x);
		min_y = std::min(chunk.y, min_y);
		min_z = std::min(chunk.z, min_z);
	}
	TIME_END("chunk decision");

	if (for_real.size() == 0)
		return;

	// generate texture needed to sample all the chunks
	min_x *= CHUNK_SIZE; max_x = (max_x + 1) * CHUNK_SIZE;
	min_y *= CHUNK_SIZE; max_y = (max_y + 1) * CHUNK_SIZE;
	min_z *= CHUNK_SIZE; max_z = (max_z + 1) * CHUNK_SIZE;
	int size_x = max_x - min_x;
	int size_y = max_y - min_y;
	int size_z = max_z - min_z;
	const glm::uvec3 tex_sz(size_x, size_y, size_z);

	TIME_BEGIN;
	HastyNoise::FloatBuffer texture = map->noise->GetNoiseSet(min_x, min_z, min_y, size_x, size_z, size_y);
	TIME_END("texture generation");

	// generate chunks (heavy lifting -- go wide)
	// TODO: this is temporary, we will eventually want to be more systematic about splitting
	//  the chunk-generating workload.
	//  it's probably a good idea to avoid such spiked work (we're idling most of the time except
	//   when we're loading chunks) and have background threads loading nearby chunks into the cache.
	//   then, walking around would be seamless. only flying really fast is an issue.
	//  at any rate, simply decoupling this work from the main render thread would be progress; allowing
	//   other systems to do their work, and joining right before we materialize the frame changes.
	TIME_BEGIN;
#pragma omp parallel for num_threads(4)
	for (int i = 0; i < for_real.size(); i++) {
		const map_system_t::chunk_t& chunk = for_real[i].second;
		const glm::ivec3& chunk_coord = for_real[i].first;
		IndexedMesh* mesh = map->ctx->assets.get<IndexedMesh>(chunk.mesh_asset);
		const glm::uvec3 offset = CHUNK_SIZE * chunk_coord - glm::ivec3(min_x, min_y, min_z);
		generate_chunk(map, chunk.mesh_asset, mesh, offset, tex_sz, texture.get());
	}
	TIME_END("heavy lifting");

	// insert chunk components
	TIME_BEGIN;
	for (int i = 0; i < for_real.size(); i++) {
		const map_system_t::chunk_t& chunk = for_real[i].second;
		const glm::ivec3 &chunk_coord = for_real[i].first;
		map->ctx->emgr.insert_component<IndexedRenderMesh>(chunk.entity, { chunk.mesh_asset });
		map->ctx->emgr.insert_component<Position>(chunk.entity, {
			(float)CHUNK_SIZE * glm::vec3(chunk_coord)
		});
	}
	TIME_END("component insertion");
}

static std::vector<glm::ivec3> get_chunks_at(map_system_t* map, glm::ivec3& position, int distance_begin, int distance_end, std::vector<glm::ivec3> &offsets, std::vector<glm::ivec3> &masks)
{
	std::vector<glm::ivec3> to_load;
	for (size_t i = 0; i < offsets.size(); i++) {
		glm::ivec3& offset = offsets[i];
		glm::ivec3& mask = masks[i];
		for (int distance = distance_begin; distance <= distance_end; distance++) {
			for (int j = -distance; j <= distance; j++) {
				to_load.push_back(distance * offset + j * mask + position);
			}
		}
	}
	return to_load;
}

void map_system_t::init(entity_t camera)
{
	packed_vertex_data = gen_packed_cube();

	seed = generate_seed();
	HastyNoise::loadSimd("./");
	noise = HastyNoise::CreateNoise(seed, HastyNoise::GetFastestSIMD());
	noise->SetNoiseType(HastyNoise::NoiseType::SimplexFractal);
	noise->SetFrequency(0.05f);

	std::printf("[map] seed=%u\n", seed);
	std::printf("[map] view_distance=%zu\n", view_distance);

	Camera& cam = ctx->emgr.get_component<Camera>(camera);
	chunk_coord = get_chunk_pos(cam.pos);

	std::vector<glm::ivec3> offsets = { glm::ivec3(0), glm::ivec3(1, 0, 0), glm::ivec3(0, 0, 1), glm::ivec3(-1, 0, 0), glm::ivec3(0, 0, -1) };
	std::vector<glm::ivec3> masks = { glm::ivec3(0), glm::ivec3(0, 0, 1), glm::ivec3(1, 0, 0), glm::ivec3(0, 0, 1), glm::ivec3(1, 0, 0) };
	auto to_load = get_chunks_at(this, chunk_coord, 0, (int)view_distance, offsets, masks);
	load_chunks(this, to_load);
}

void map_system_t::update(entity_t camera)
{
	Camera& cam = ctx->emgr.get_component<Camera>(camera);
	glm::ivec3 new_chunk_coord = get_chunk_pos(cam.pos);
	if (chunk_coord == new_chunk_coord)
		return;

	////////////////////////////////////////////////////
	// first, sort our chunk cache to optimally 
	//  choose what chunks to evict
	////////////////////////////////////////////////////

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


	////////////////////////////////////////////////////
	// compute coordinates of the chunks we want to load
	////////////////////////////////////////////////////

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

	auto to_load = get_chunks_at(this, new_chunk_coord, (int)view_distance, (int)view_distance, offsets, masks);

	// and finally load them
	load_chunks(this, to_load);
	chunk_coord = new_chunk_coord;
}
