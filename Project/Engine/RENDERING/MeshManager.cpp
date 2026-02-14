#include "RenderingComponents.h"

namespace RENDERING
{
    MeshManager& MeshManager::Instance()
    {
        static MeshManager instance;
        return instance;
    }

    uint32_t MeshManager::LoadMesh(const std::string& name, const std::string& filepath)
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = nameToId.find(name);
        if (it != nameToId.end())
            return it->second;

        std::vector<FBXVertex> vertices;
        std::vector<uint32_t> index;
        if (!UTILITIES::LoadFBXMesh(filepath, vertices, index))
            return UINT32_MAX;

        resources.emplace_back();
        auto& newResourses = resources.back();

        RENDERING::RENDERER_HELPERS::Memory::CreateVertexAndIndexBuffersFor(
            *RENDERER_HELPERS::GetGlobalRenderer(), vertices, index,
            newResourses.vertexBuffer, newResourses.vertexBufferMemory,
            newResourses.indexBuffer, newResourses.indexBufferMemory, newResourses.indexCount);

        // move CPU data into the same element
        newResourses.vertices = std::move(vertices);
        newResourses.indices = std::move(index);

        // id is the index of the newly added resource
        uint32_t id = static_cast<uint32_t>(resources.size() - 1);
        nameToId[name] = id;
        return id;
    }

    uint32_t MeshManager::GetMeshId(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = nameToId.find(name);
        return (it == nameToId.end()) ? UINT32_MAX : it->second;
    }

    const MeshResource* MeshManager::GetMesh(uint32_t id) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (id >= resources.size()) return nullptr;
        return &resources[id];
    }

    size_t MeshManager::MeshCount() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return resources.size();
    }

    void MeshManager::ReleaseCpuMeshData(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (id >= resources.size()) return;
        resources[id].vertices.clear();
        resources[id].vertices.shrink_to_fit();
        resources[id].indices.clear();
        resources[id].indices.shrink_to_fit();
    }
}