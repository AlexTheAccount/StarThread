#include "UtilityComponents.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace UTILITIES;

namespace UTILITIES
{
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

        outVertices.clear();
        outIndices.clear();

        size_t totalVertices = 0;
        size_t totalFaces = 0;
        for (unsigned int mesh = 0; mesh < scene->mNumMeshes; ++mesh)
        {
            totalVertices += scene->mMeshes[mesh]->mNumVertices;
            totalFaces += scene->mMeshes[mesh]->mNumFaces;
        }
        outVertices.reserve(totalVertices);
        if (totalFaces)
            outIndices.reserve(totalFaces * 3);

        // Merge all meshes into one vertex/index buffer
        uint32_t vertexOffset = 0;
        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            aiMesh* mesh = scene->mMeshes[meshIndex];
            if (!mesh) continue;

            // Append vertices
            for (unsigned int meshVertex = 0; meshVertex < mesh->mNumVertices; ++meshVertex)
            {
                FBXVertex vertex{};
                if (mesh->mVertices)
                {
                    vertex.position[0] = mesh->mVertices[meshVertex].x;
                    vertex.position[1] = mesh->mVertices[meshVertex].y;
                    vertex.position[2] = mesh->mVertices[meshVertex].z;
                }
                if (mesh->mNormals)
                {
                    vertex.normal[0] = mesh->mNormals[meshVertex].x;
                    vertex.normal[1] = mesh->mNormals[meshVertex].y;
                    vertex.normal[2] = mesh->mNormals[meshVertex].z;
                }
                if (mesh->mTextureCoords[0])
                {
                    vertex.uv[0] = mesh->mTextureCoords[0][meshVertex].x;
                    vertex.uv[1] = mesh->mTextureCoords[0][meshVertex].y;
                }
                else
                {
                    vertex.uv[0] = vertex.uv[1] = 0.0f;
                }
                outVertices.push_back(vertex);
            }

            // Append indices
            for (unsigned int meshFace = 0; meshFace < mesh->mNumFaces; ++meshFace)
            {
                const aiFace& face = mesh->mFaces[meshFace];
                if (face.mNumIndices == 3)
                {
                    outIndices.push_back(static_cast<uint32_t>(face.mIndices[0]) + vertexOffset);
                    outIndices.push_back(static_cast<uint32_t>(face.mIndices[1]) + vertexOffset);
                    outIndices.push_back(static_cast<uint32_t>(face.mIndices[2]) + vertexOffset);
                }
            }

            vertexOffset += mesh->mNumVertices;
        }

        return true;
    }
}