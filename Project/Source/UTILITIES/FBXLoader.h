#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct FBXVertex
{
    float position[3];
    float normal[3];
    float uv[2];
};

bool LoadFBXMesh(const std::string& filepath, std::vector<FBXVertex>& outVertices, std::vector<uint32_t>& outIndices);