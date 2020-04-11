#include "MapSystem.h"
#include "Position.h"
#include "Camera.h"
#include <random>
#include <sstream>
#include "Context.h"
#include "IndexedMesh.h"
#include "RenderModel.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <omp.h>
#include "utils.h"
#include <Tracy.hpp>
#include <bitset>

static const int CHUNK_SIZE = 64;
static const size_t CHUNK_1 = static_cast<size_t>(CHUNK_SIZE);
static const size_t CHUNK_2 = CHUNK_1 * CHUNK_1;
static const size_t CHUNK_3 = CHUNK_2 * CHUNK_1;

#define VEC3_UNPACK(v) v.x, v.y, v.z
#define VEC3_FMTD "(%d, %d, %d)"

map_system_t::map_system_t(context_t* ctx)
	: ctx(ctx), view_distance(5), seed(0), chunk_coord(0), n_chunks(0)
{}

map_system_t::~map_system_t()
{}

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
////////////////////////////////////////////

struct quad_t {
	unsigned x, y, z, w, h, d;
};

//static thread_local std::bitset<CHUNK_3> greedy_bitset;
static std::vector<quad_t> blocks_to_mesh_greedy(const int *blocks, int type)
{
	std::bitset<CHUNK_3> greedy_bitset;
	greedy_bitset.reset();

	auto bit_at = [&greedy_bitset](size_t x, size_t y, size_t z) {
		return greedy_bitset[CHUNK_2 * x + CHUNK_1 * z + y];
	};

	// initialize bitset
	for (size_t i = 0; i < CHUNK_3; i++) {
		greedy_bitset[i] = blocks[i] == type;
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

static glm::vec3 block_color(const int type)
{
	switch (type) {
	case chunk_t::GRASS:
		return glm::vec3(0.45f, 0.74f, 0.45f);
	case chunk_t::ROCK:
		return glm::vec3(0.4f);
	default:
		return glm::vec3(1.f, 0.f, 0.f);
	}
}

static void generate_quad_mesh(map_system_t* map, const asset_t a, const std::vector<quad_t>& quads, int type)
{
	auto mesh = map->ctx->assets.get<IndexedMesh>(a);
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

	glm::vec3 color = block_color(type);

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

static void generate_chunk(map_system_t *map, chunk_t& chunk, glm::ivec3 coordinate)
{
	ZoneScoped;

	bool is_outside_range = coordinate.y >= 1 || coordinate.y < -1;
	glm::ivec3 tex_coord = (int)CHUNK_SIZE * coordinate;

	HastyNoise::FloatBuffer texture;
	{
		ZoneScoped("noise_gen");
		map->noise->SetNoiseType(HastyNoise::NoiseType::Simplex);
		map->noise->SetFrequency(0.05f);
		if (!is_outside_range)
			texture = map->noise->GetNoiseSet(tex_coord.x, tex_coord.z, tex_coord.y, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
	}

	HastyNoise::FloatBuffer terrain;
	{
		ZoneScoped("terrain_gen");
		map->noise->SetNoiseType(HastyNoise::NoiseType::Simplex);
		map->noise->SetFrequency(0.0025f);
		if (!is_outside_range)
			terrain = map->noise->GetNoiseSet(tex_coord.x, tex_coord.z, 0, CHUNK_SIZE, CHUNK_SIZE, 1);
	}

	float* buffer = texture.get();
	int* blocks = new int[CHUNK_3];

	for (size_t x = 0; x < CHUNK_SIZE; x++) {
		for (size_t z = 0; z < CHUNK_SIZE; z++) {
			for (size_t y = 0; y < CHUNK_SIZE; y++) {
				const size_t idx = x * CHUNK_2 + z * CHUNK_1 + y;
				const size_t terrain_idx = x * CHUNK_1 + z;
				const float test = (float)coordinate.y + (float)y / (float)CHUNK_SIZE;

				if (is_outside_range) {
					blocks[idx] = test > 0 ? chunk_t::AIR : chunk_t::GRASS;
					continue;
				}

				const float elevation = std::pow(terrain.get()[terrain_idx], 1.f);
				if (test > elevation) {
					blocks[idx] = chunk_t::AIR;
				}
				else {
					blocks[idx] = chunk_t::GRASS;
				}
			}
		}
	}

	{
		ZoneScoped("greedy_mesh");
		std::vector<quad_t> quads;
		for (int j = 0; j < chunk_t::_COUNT; j++) {
			quads = blocks_to_mesh_greedy(blocks, j);
			generate_quad_mesh(map, chunk.meshes[j], quads, j);
		}
	}

	delete[] blocks;
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
	pos.y = static_cast<int>(std::floor(world_pos.y / static_cast<float>(CHUNK_SIZE)));
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
	const size_t view_on_flight = (2 * map->view_distance + 1) * (2 * map->view_distance + 1) * (2 * map->view_distance + 1);
	std::vector<std::pair<glm::ivec3, chunk_t>> for_real;
	std::stringstream ss;
	static uint32_t base = 0;
	{
		ZoneScoped("chunk_prepare");
		for (int i = 0; i < to_load.size(); i++) {
			glm::ivec3 chunk = to_load[i];
			chunk_t ch;
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
				map->ctx->emgr.new_entity(&ch.entity, 1);
				for (int j = 0; j < chunk_t::_COUNT; j++) {
					ss.str(std::string());
					ss.clear();
					ss << "ChunkMesh" << base++;
					ch.meshes[j] = hash_str(ss.str());
					IndexedMesh* mesh = map->ctx->assets.make<IndexedMesh>(ch.meshes[j]);
					mesh->vertices = nullptr;
					mesh->indices = nullptr;
					mesh->num_indices = 0;
					mesh->num_verts = 0;
				}
				map->chunk_cache.insert({ chunk, ch });
			}
			for_real.push_back({ chunk, ch });
		}
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
	// ANSWER: this is in fact thread safe --- each thread touches the memory blocks of a given number of assets,
	//         and they never insert/remove from the outermost map in the asset manager. so it's ok.
#pragma omp parallel for num_threads(4)
	for (int i = 0; i < for_real.size(); i++) {
		generate_chunk(map, for_real[i].second, for_real[i].first);
	}

	// insert chunk components and refill cache vector
	for (int i = 0; i < for_real.size(); i++) {
		const chunk_t& chunk = for_real[i].second;
		const glm::ivec3& chunk_coord = for_real[i].first;
		map->chunk_cache_sorted.push_back(chunk_coord);
		RenderModel model;
		std::memcpy(model.meshes, chunk.meshes, chunk_t::_COUNT * sizeof(asset_t));
		model.num_meshes = chunk_t::_COUNT;
		model.last_update = SDL_GetTicks();
		model.visible = true;
		map->ctx->emgr.insert_component<RenderModel>(chunk.entity, model);
		map->ctx->emgr.insert_component<Position>(chunk.entity, { (float)CHUNK_SIZE * glm::vec3(chunk_coord) });
	}
}

void map_system_t::init(entity_t camera)
{
	ZoneScoped;

	seed = generate_seed();
	HastyNoise::loadSimd("./");
	noise = HastyNoise::CreateNoise(seed, 3);

	std::printf("[map] seed=%u\n", seed);
	std::printf("[map] view_distance=%zu\n", view_distance);

	Camera& cam = ctx->emgr.get_component<Camera>(camera);
	chunk_coord = get_chunk_pos(cam.pos);

	std::vector<glm::ivec3> to_load;
	int vdist = static_cast<int>(view_distance);
	for (int i = -vdist; i < vdist; i++) {
		for (int j = -vdist; j < vdist; j++) {
			for (int k = -vdist; k < vdist; k++) {
				to_load.emplace_back(chunk_coord + glm::ivec3(i, j, k));
			}
		}
	}
	load_chunks(this, to_load);
}

void map_system_t::update(entity_t camera)
{
	Camera& cam = ctx->emgr.get_component<Camera>(camera);
	glm::ivec3 new_chunk_pos = get_chunk_pos(cam.pos);
	if (chunk_coord == new_chunk_pos)
		return;

	chunk_coord = new_chunk_pos;
	std::vector<glm::ivec3> to_load;
	int vdist = static_cast<int>(view_distance);
	for (int i = -vdist; i < vdist; i++) {
		for (int j = -vdist; j < vdist; j++) {
			for (int k = -vdist; k < vdist; k++) {
				to_load.emplace_back(glm::ivec3(i, j, k) + chunk_coord);
			}
		}
	}

	{
		ZoneScoped;
		load_chunks(this, to_load);
	}

	// also, hide chunks that are too far away
	for (auto& ch : chunk_cache) {
		const chunk_t& chunk = ch.second;
		if (!ctx->emgr.has_component<RenderModel>(chunk.entity))
			continue;

		glm::ivec3 dist = glm::abs(ch.first - chunk_coord);
		const bool visible = dist.x <= view_distance && dist.y <= view_distance && dist.z <= view_distance;
		auto& rm = ctx->emgr.get_component<RenderModel>(chunk.entity);
		if (rm.visible == visible)
			continue;

		RenderModel model;
		std::memcpy(model.meshes, chunk.meshes, chunk_t::_COUNT * sizeof(asset_t));
		model.num_meshes = chunk_t::_COUNT;
		model.last_update = SDL_GetTicks();
		model.visible = visible;

		ctx->emgr.insert_component<RenderModel>(chunk.entity, model);
	}
}
