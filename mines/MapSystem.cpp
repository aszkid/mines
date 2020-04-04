#include "MapSystem.h"
#include "Chunk.h"
#include <random>
#include "Context.h"
#include "Mesh.h"
#include "RenderMesh.h"
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

struct tmp_block_t {
	glm::vec3 pos;
	int type;
};

static void generate_chunk_mesh(asset_manager_t *assets, asset_t a, std::vector<tmp_block_t> &blocks)
{
	Mesh* mesh = assets->make<Mesh>(a);
	// each block needs 36 vertices
	mesh->num_verts = 36 * blocks.size();
	mesh->vertices = assets->allocate_chunk<Mesh::Vertex>(a, mesh->num_verts);
	Mesh::Vertex* v = mesh->vertices;

#define UP_NORMAL v->nx = 0.f; v->ny = 1.f; v->nz = 0.f
#define R_NORMAL v->nx = 1.f; v->ny = 0.f; v->nz = 0.f
#define L_NORMAL v->nx = -1.f; v->ny = 0.f; v->nz = 0.f
#define DOWN_NORMAL v->nx = 0.f; v->ny = -1.f; v->nz = 0.f
#define INTO_NORMAL v->nx = 0.f; v->ny = 0.f; v->nz = -1.f
#define FROM_NORMAL v->nx = 0.f; v->ny = 0.f; v->nz = 1.f

	for (tmp_block_t& blk : blocks) {
		glm::vec3& pos = blk.pos;
		// tri 1
		v->x = pos.x; v->y = pos.y; v->z = pos.z; DOWN_NORMAL; v += 1;
		v->x = pos.x; v->y = pos.y; v->z = pos.z + 1.f; DOWN_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z; DOWN_NORMAL; v += 1;
		// tri 2
		v->x = pos.x; v->y = pos.y; v->z = pos.z + 1.f; DOWN_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z + 1.f; DOWN_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z; DOWN_NORMAL; v += 1;
		// tri 3
		v->x = pos.x; v->y = pos.y; v->z = pos.z; L_NORMAL; v += 1;
		v->x = pos.x; v->y = pos.y; v->z = pos.z + 1.f; L_NORMAL; v += 1;
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z; L_NORMAL; v += 1;
		// tri 4
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z; L_NORMAL; v += 1;
		v->x = pos.x; v->y = pos.y; v->z = pos.z + 1.f; L_NORMAL; v += 1;
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z + 1.f; L_NORMAL; v += 1;
		// tri 5
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z + 1.f; FROM_NORMAL; v += 1;
		v->x = pos.x; v->y = pos.y; v->z = pos.z + 1.f; FROM_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z + 1.f; FROM_NORMAL; v += 1;
		// tri 6
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z + 1.f; FROM_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z + 1.f; FROM_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z + 1.f; FROM_NORMAL; v += 1;
		// tri 7 (3+x)
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z; R_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z + 1.f; R_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z; R_NORMAL; v += 1;
		// tri 8 (4+x)
		v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z; R_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z + 1.f; R_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z + 1.f; R_NORMAL; v += 1;
		// tri 9 (5-z)
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z; INTO_NORMAL; v += 1;
		v->x = pos.x; v->y = pos.y; v->z = pos.z; INTO_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z; INTO_NORMAL; v += 1;
		// tri 10 (6-z)
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z; INTO_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y; v->z = pos.z; INTO_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z; INTO_NORMAL; v += 1;
		// tri 11 (1+y)
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z; UP_NORMAL; v += 1;
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z + 1.f; UP_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z; UP_NORMAL; v += 1;
		// tri 12 (2+y)
		v->x = pos.x; v->y = pos.y + 1.f; v->z = pos.z + 1.f; UP_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z + 1.f; UP_NORMAL; v += 1;
		v->x = pos.x + 1.f; v->y = pos.y + 1.f; v->z = pos.z; UP_NORMAL; v += 1;
	}
}

void map_system_t::init()
{

	chunk_t chunk;
	entity_t ch[2];
	ctx->emgr.new_entity(ch, 2);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dis;
	seed = dis(gen);
	std::printf("[map] seed=%u\n", seed);

	// generate asset storage space for grass blocks
	asset_t grass = "GrassMesh"_hash;
	std::vector<tmp_block_t> grass_blocks;
	asset_t rock = "RockMesh"_hash;
	std::vector<tmp_block_t> rock_blocks;

	const size_t N = 16;
	HastyNoise::loadSimd("./");
	size_t fastestSIMD = HastyNoise::GetFastestSIMD();
	auto noise = HastyNoise::CreateNoise(seed, fastestSIMD);
	noise->SetNoiseType(HastyNoise::NoiseType::Perlin);
	noise->SetFrequency(0.1f);
	HastyNoise::FloatBuffer buffer = noise->GetNoiseSet(0, 0, 0, N, N, N);

	size_t idx = 0;
	for (size_t x = 0; x < N; x++) {
		for (size_t y = 0; y < N; y++) {
			for (size_t z = 0; z < N; z++) {
				float val = buffer.get()[idx];
				if (val > 0.f && val < .2f) {
					chunk.blocks[x][y][z].type = chunk_t::block_t::GRASS;
					grass_blocks.emplace_back(tmp_block_t { glm::vec3(x, y, z), chunk_t::block_t::GRASS });
				} else if (val > .2f) {
					chunk.blocks[x][y][z].type = chunk_t::block_t::ROCK;
					rock_blocks.emplace_back(tmp_block_t{ glm::vec3(x, y, z), chunk_t::block_t::ROCK });
				} else {
					chunk.blocks[x][y][z].type = chunk_t::block_t::AIR;
				}
				idx++;
			}
		}
	}

	std::printf("[map] have %zu grass, %zu rock\n", grass_blocks.size(), rock_blocks.size());
	// THIS IS BAD
	ctx->emgr.insert_component<chunk_t>(ch[0], std::move(chunk));
	ctx->emgr.insert_component<chunk_t>(ch[1], std::move(chunk));

	generate_chunk_mesh(&ctx->assets, grass, grass_blocks);
	generate_chunk_mesh(&ctx->assets, rock, rock_blocks);
	ctx->emgr.insert_component<RenderMesh>(ch[0], { grass, glm::vec3(0.f, 1.f, 0.f) });
	ctx->emgr.insert_component<RenderMesh>(ch[1], { rock, glm::vec3(.4f, .4f, .4f) });
}

void map_system_t::update()
{
}
