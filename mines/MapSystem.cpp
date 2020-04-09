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
#include "utils.h"
#include <Tracy.hpp>
#include <bitset>

static const int CHUNK_SIZE = 32;
static const size_t CHUNK_1 = static_cast<size_t>(CHUNK_SIZE);
static const size_t CHUNK_2 = CHUNK_1 * CHUNK_1;
static const size_t CHUNK_3 = CHUNK_2 * CHUNK_1;

#define VEC3_UNPACK(v) v.x, v.y, v.z

map_system_t::map_system_t(context_t* ctx)
	: ctx(ctx), view_distance(7), seed(0), chunk_coord(0), n_chunks(0)
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

struct quad_t {
	unsigned x, y, z, w, h, d;
};

static thread_local std::bitset<CHUNK_3> greedy_bitset;
static std::vector<quad_t> blocks_to_mesh_greedy(const int *blocks)
{
	greedy_bitset.reset();

	auto bit_at = [](size_t x, size_t y, size_t z) {
		return greedy_bitset[CHUNK_2 * x + CHUNK_1 * z + y];
	};

	// initialize bitset
	for (size_t i = 0; i < CHUNK_3; i++) {
		greedy_bitset[i] = blocks[i] != 0;
	}
	
	///////////////////////////////////////////////////////
	// TODO: should build quads from different corners in
	//       the chunk and then stick with the best result.

	// sweep 2D planes from y=0 to y=CHUNK_SIZE-1
	std::vector<quad_t> quads;
	for (unsigned y = 0; y < CHUNK_1; y++) {
		for (unsigned z = 0; z < CHUNK_1; z++) {
			for (unsigned x = 0; x < CHUNK_1; x++) {
				// find a set block
				if (!bit_at(x, y, z))
					continue;
				// otherwise, generate quad starting at (x, y, z)
				// stretch the x axis
				unsigned i;
				for (i = x; i < CHUNK_1 && bit_at(i, y, z); i++) {}
				unsigned w = i - x;
				// stretch the z axis
				unsigned j;
				for (j = z + 1; j < CHUNK_1; j++) {
					for (i = x; i < x + w && bit_at(i, y, j); i++) {}
					if (i != x + w)
						break;
				}
				unsigned d = j - z;
				// stretch the y axis
				unsigned k;
				for (k = y + 1; k < CHUNK_1; k++) {
					for (j = z; j < z + d; j++) {
						for (i = x; i < x + w && bit_at(i, k, j); i++) {}
						if (i != x + w)
							break;
					}
					if (j != z + d)
						break;
				}
				unsigned h = k - y;

				// submit quad
				quad_t q = { x, y, z, w, h, d };
				quads.push_back(q);

				// mark bitmask
				for (i = x; i < x + w; i++) {
					for (j = z; j < z + d; j++) {
						for (k = y; k < y + h; k++) {
							bit_at(i, k, j) = false;
						}
					}
				}
			}
		}
	}
	return quads;
}


static void generate_chunk_mesh(map_system_t* map, IndexedMesh *mesh, asset_t a, std::vector<tmp_block_t> &blocks)
{
	size_t old_num_verts = mesh->num_verts;
	mesh->num_verts = 24 * blocks.size();
	mesh->num_indices = 36 * blocks.size();

	// allocate memory if needed
	const size_t old_chunk_sz = map->ctx->assets.get_chunk_size(a, (uint8_t*)mesh->vertices) / sizeof(IndexedMesh::Vertex);
	if (mesh->vertices == nullptr || old_chunk_sz < mesh->num_verts) {
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


static void generate_quad_mesh(map_system_t* map, IndexedMesh* mesh, const asset_t a, const std::vector<quad_t>& quads)
{
	size_t old_num_verts = mesh->num_verts;
	mesh->num_verts = 24 * quads.size();
	mesh->num_indices = 36 * quads.size();

	// allocate memory if needed
	const size_t old_chunk_sz = map->ctx->assets.get_chunk_size(a, (uint8_t*)mesh->vertices) / sizeof(IndexedMesh::Vertex);
	if (mesh->vertices == nullptr || old_chunk_sz < mesh->num_verts) {
		map->ctx->assets.free_chunk(a, (uint8_t*)mesh->vertices);
		map->ctx->assets.free_chunk(a, (uint8_t*)mesh->indices);
		mesh->vertices = map->ctx->assets.allocate_chunk<IndexedMesh::Vertex>(a, mesh->num_verts);
		mesh->indices = map->ctx->assets.allocate_chunk<unsigned int>(a, mesh->num_indices);
	}

	const glm::vec3 color = glm::vec3(0.45f, 0.74f, 0.45f);
	for (size_t k = 0; k < quads.size(); k++) {
		const quad_t& q = quads[k];
		const glm::vec3 pos = glm::vec3(q.x, q.y, q.z);

		// vertex data
		const glm::vec3 quad_vs[8] = {
			glm::vec3(q.x, q.y, q.z),
			glm::vec3(q.x, q.y, q.z + q.d),
			glm::vec3(q.x + q.w, q.y, q.z + q.d),
			glm::vec3(q.x + q.w, q.y, q.z),
			glm::vec3(q.x, q.y + q.h, q.z),
			glm::vec3(q.x, q.y + q.h, q.z + q.d),
			glm::vec3(q.x + q.w, q.y + q.h, q.z + q.d),
			glm::vec3(q.x + q.w, q.y + q.h, q.z)
		};
		const glm::vec3 quad_ids[24] = {
			quad_vs[0], quad_vs[1], quad_vs[2], quad_vs[3],
			quad_vs[4], quad_vs[5], quad_vs[6], quad_vs[7],
			quad_vs[0], quad_vs[1], quad_vs[5], quad_vs[4],
			quad_vs[3], quad_vs[2], quad_vs[6], quad_vs[7],
			quad_vs[1], quad_vs[2], quad_vs[6], quad_vs[5],
			quad_vs[0], quad_vs[3], quad_vs[7], quad_vs[4]
		};
		for (size_t j = 0; j < 24; j++) {
			mesh->vertices[24 * k + j].x = quad_ids[j].x; mesh->vertices[24 * k + j].y = quad_ids[j].y; mesh->vertices[24 * k + j].z = quad_ids[j].z;
			mesh->vertices[24 * k + j].nx = normal_data[j].x; mesh->vertices[24 * k + j].ny = normal_data[j].y; mesh->vertices[24 * k + j].nz = normal_data[j].z;
			mesh->vertices[24 * k + j].r = color.r; mesh->vertices[24 * k + j].g = color.g; mesh->vertices[24 * k + j].b = color.b;
		}

		// indices
		std::memcpy(&mesh->indices[36 * k], indices, 36 * sizeof(unsigned int));
		for (size_t j = 0; j < 36; j++) {
			mesh->indices[36 * k + j] += 24 * k;
		}
	}
}


static void generate_chunk(map_system_t *map, asset_t asset, IndexedMesh *mesh, glm::ivec3 coordinate)
{
	ZoneScoped;

	uint32_t before, delta;
	std::vector<tmp_block_t> blocks;
	blocks.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);

	coordinate = (int)CHUNK_SIZE * coordinate;
	HastyNoise::FloatBuffer texture;
	{
		ZoneScoped("noise_gen");
		texture = map->noise->GetNoiseSet(coordinate.x, coordinate.z, coordinate.y, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
	}
	glm::uvec3 tex_offset(0);
	glm::uvec3 tex_sz(CHUNK_SIZE);
	float* buffer = texture.get();

	int* block_markers = new int[CHUNK_3];

	for (size_t x = 0; x < CHUNK_SIZE; x++) {
		for (size_t z = 0; z < CHUNK_SIZE; z++) {
			for (size_t y = 0; y < CHUNK_SIZE; y++) {
				const size_t tex_idx = (x + tex_offset.x) * tex_sz.z * tex_sz.y + (z + tex_offset.z) * tex_sz.y + y + tex_offset.y;
				const float val = buffer[tex_idx];
				if (val > 0.f && val < .2f) {
					//chunk.blocks[x][y][z].type = chunk_t::block_t::GRASS;
					block_markers[x * CHUNK_2 + z * CHUNK_1 + y] = 1;
					blocks.push_back({ glm::vec3(x, y, z), glm::vec3(0.45f, 0.74f, 0.45f) });
				}
				else if (val > .2f) {
					//chunk.blocks[x][y][z].type = chunk_t::block_t::ROCK;
					block_markers[x * CHUNK_2 + z * CHUNK_1 + y] = 2;
					blocks.push_back({ glm::vec3(x, y, z), glm::vec3(.4f, .4f, .4f) });
				}
				else {
					block_markers[x * CHUNK_2 + z * CHUNK_1 + y] = 0;
					//chunk.blocks[x][y][z].type = chunk_t::block_t::AIR;
				}
			}
		}
	}

	std::vector<quad_t> quads;
	{
		ZoneScoped("greedy");
		quads = blocks_to_mesh_greedy(block_markers);
		delete[] block_markers;
	}

	{
		ZoneScoped("generate_mesh");
		//generate_chunk_mesh(map, mesh, asset, blocks);
		generate_quad_mesh(map, mesh, asset, quads);
	}
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
	ZoneScoped;

	{
		ZoneScoped("cache_sort");
		static const auto sort_by_distance = [map](glm::ivec3& a, glm::ivec3& b) -> bool {
			glm::vec3 aa(a); glm::vec3 bb(b);
			glm::vec3 pos(map->chunk_coord);
			return glm::distance(pos, aa) < glm::distance(pos, bb);
		};
		std::sort(map->chunk_cache_sorted.begin(), map->chunk_cache_sorted.end(), sort_by_distance);
	}


	// load chunk coordinates, ignore hot chunks
	static const size_t view_on_flight = (2 * map->view_distance + 1) * (2 * map->view_distance + 1);
	std::vector<std::pair<glm::ivec3, map_system_t::chunk_t>> for_real;
	{
		ZoneScoped("chunk_prepare");
		for (int i = 0; i < to_load.size(); i++) {
			glm::ivec3& chunk = to_load[i];
			map_system_t::chunk_t ch;
			if (map->chunk_cache.find(chunk) != map->chunk_cache.end())
				continue;
			// this is the max cache size we're willing to deal with
			// TODO should depend on the view distance, but how exactly?????
			if (map->chunk_cache_sorted.size() > view_on_flight * 2) {
				glm::ivec3 target = map->chunk_cache_sorted.back();
				map->chunk_cache_sorted.pop_back();
				auto nh = map->chunk_cache.extract(target);
				ch = nh.mapped();
				nh.key() = chunk;
				map->chunk_cache.insert(std::move(nh));
			} else {
				std::stringstream ss;
				static uint32_t base = 0;
				ss << "ChunkMesh" << base++;
				ch.mesh_asset = hash_str(ss.str());
				map->ctx->emgr.new_entity(&ch.entity, 1);
				IndexedMesh* mesh = map->ctx->assets.make<IndexedMesh>(ch.mesh_asset);
				mesh->vertices = nullptr;
				mesh->indices = nullptr;
				mesh->num_indices = 0;
				mesh->num_verts = 0;
				map->chunk_cache.insert({ chunk, ch });
			}
			for_real.push_back({ chunk, ch });
		}

		if (for_real.size() == 0)
			return;
	}

	// generate chunks (heavy lifting -- go wide)
	// TODO: this is temporary, we will eventually want to be more systematic about splitting
	//  the chunk-generating workload.
	//  it's probably a good idea to avoid such spiked work (we're idling most of the time except
	//   when we're loading chunks) and have background threads loading nearby chunks into the cache.
	//   then, walking around would be seamless. only flying really fast is an issue.
	//  at any rate, simply decoupling this work from the main render thread would be progress; allowing
	//   other systems to do their work, and joining right before we materialize the frame changes.
	// TODO2: this is thread unsafe -- we are allocating stuff from the asset manager,
	//        which right now has no locking.
#pragma omp parallel for num_threads(4)
	for (int i = 0; i < for_real.size(); i++) {
		const map_system_t::chunk_t& chunk = for_real[i].second;
		IndexedMesh* mesh = map->ctx->assets.get<IndexedMesh>(chunk.mesh_asset);
		generate_chunk(map, chunk.mesh_asset, mesh, for_real[i].first);
	}

	// insert chunk components and refill cache vector
	for (int i = 0; i < for_real.size(); i++) {
		const map_system_t::chunk_t& chunk = for_real[i].second;
		const glm::ivec3 &chunk_coord = for_real[i].first;
		map->chunk_cache_sorted.push_back(chunk_coord);
		map->ctx->emgr.insert_component<IndexedRenderMesh>(chunk.entity, { chunk.mesh_asset, true, SDL_GetTicks() });
		map->ctx->emgr.insert_component<Position>(chunk.entity, {
			(float)CHUNK_SIZE * glm::vec3(chunk_coord)
		});
	}
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
	noise->SetNoiseType(HastyNoise::NoiseType::Simplex);
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

	// and load them
	auto to_load = get_chunks_at(this, new_chunk_coord, (int)view_distance, (int)view_distance, offsets, masks);
	chunk_coord = new_chunk_coord;
	{
		ZoneScoped;
		load_chunks(this, to_load);
	}

	// also, hide chunks that are too far away
	const uint32_t time = SDL_GetTicks();
	for (auto& ch : chunk_cache) {
		glm::ivec3 dist = glm::abs(ch.first - chunk_coord);
		bool visible = !(dist.x > view_distance || dist.y > view_distance || dist.z > view_distance);
		uint32_t last_update = time;
		if (ctx->emgr.has_component<IndexedRenderMesh>(ch.second.entity)) {
			auto& rm = ctx->emgr.get_component<IndexedRenderMesh>(ch.second.entity);
			last_update = rm.last_update;
			if (visible == rm.visible)
				continue;
		}
		ctx->emgr.insert_component<IndexedRenderMesh>(ch.second.entity, { ch.second.mesh_asset, visible, last_update });
	}
}
