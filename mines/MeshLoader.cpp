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

    mesh_->num_verts = mesh->mNumVertices;
    mesh_->vertices = amgr->allocate_chunk<Mesh::Vertex>(id, mesh->mNumVertices);
    for (size_t i = 0; i < mesh->mNumVertices; i++) {
        mesh_->vertices[i].x = mesh->mVertices[i].x;
        mesh_->vertices[i].y = mesh->mVertices[i].y;
        mesh_->vertices[i].z = mesh->mVertices[i].z;
        mesh_->vertices[i].nx = mesh->mNormals[i].x;
        mesh_->vertices[i].ny = mesh->mNormals[i].y;
        mesh_->vertices[i].nz = mesh->mNormals[i].z;
    }

    return 0;
}