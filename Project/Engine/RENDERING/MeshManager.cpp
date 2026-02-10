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

        MeshResource resource;
        resource.vertices = std::move(vertices);
        resource.indices = std::move(index);

        uint32_t id = static_cast<uint32_t>(resources.size());
        resources.push_back(std::move(resource));
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