#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "AssetManager.h"
#include "Mesh.h"

static int load_mesh(asset_manager_t *amgr, asset_t id, const char* path)
{
    Mesh* mesh_ = amgr->make<Mesh>(id);

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::printf("[load_mesh] assimp error: %s\n", importer.GetErrorString());
        return -1;
    }

    // process first mesh only
    aiNode* node = scene->mRootNode->mChildren[0];
    aiMesh* mesh = scene->mMeshes[node->mMeshes[0]];

    aiVector3D *data = (aiVector3D*)amgr->allocate_chunk(id, 2 * sizeof(aiVector3D) * mesh->mNumVertices);
    mesh_->vertices = data;
    mesh_->normals = data + mesh->mNumVertices;

    std::memcpy(mesh_->vertices, mesh->mVertices, sizeof(aiVector3D) * mesh->mNumVertices);
    std::memcpy(mesh_->normals, mesh->mNormals, sizeof(aiVector3D) * mesh->mNumVertices);
    mesh_->num_verts = mesh->mNumVertices;

    return 0;
}