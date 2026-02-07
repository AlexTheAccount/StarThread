#include "FBXLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

bool LoadFBXMesh(const std::string& filepath, std::vector<FBXVertex>& outVertices, std::vector<uint32_t>& outIndices)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality);

    if (!scene || !scene->HasMeshes())
        return false;

    // load first mesh
    aiMesh* mesh = scene->mMeshes[0];

    outVertices.clear();
    outIndices.clear();
    outVertices.reserve(mesh->mNumVertices);
    if (mesh->mNumFaces)
        outIndices.reserve(mesh->mNumFaces * 3);

    for (unsigned int numVertice = 0; numVertice < mesh->mNumVertices; ++numVertice)
    {
        FBXVertex vertex{};
        if (mesh->mVertices)
        {
            vertex.position[0] = mesh->mVertices[numVertice].x;
            vertex.position[1] = mesh->mVertices[numVertice].y;
            vertex.position[2] = mesh->mVertices[numVertice].z;
        }
        if (mesh->mNormals)
        {
            vertex.normal[0] = mesh->mNormals[numVertice].x;
            vertex.normal[1] = mesh->mNormals[numVertice].y;
            vertex.normal[2] = mesh->mNormals[numVertice].z;
        }
        if (mesh->mTextureCoords[0])
        {
            vertex.uv[0] = mesh->mTextureCoords[0][numVertice].x;
            vertex.uv[1] = mesh->mTextureCoords[0][numVertice].y;
        }
        else
        {
            vertex.uv[0] = vertex.uv[1] = 0.0f;
        }
        outVertices.push_back(vertex);
    }

    for (unsigned int numFace = 0; numFace < mesh->mNumFaces; ++numFace)
    {
        const aiFace& face = mesh->mFaces[numFace];
        if (face.mNumIndices == 3)
        {
            outIndices.push_back(face.mIndices[0]);
            outIndices.push_back(face.mIndices[1]);
            outIndices.push_back(face.mIndices[2]);
        }
    }

    return true;
}