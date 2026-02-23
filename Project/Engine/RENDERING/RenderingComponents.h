#pragma once
#define GLFW_INCLUDE_VULKAN
#include "../UTILITIES/UtilityComponents.h"
#include <cstdint>
#include <optional>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <entt/entt.hpp>
#include <unordered_map>
#include <mutex>

using namespace UTILITIES;

namespace RENDERING
{
    //*** TAGS ***//
    struct RenderableTag {};

    //*** COMPONENTS ***//
    struct MeshResource
    {
        std::vector<FBXVertex> vertices;
        std::vector<uint32_t> indices;
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
        size_t indexCount = 0;
    };

    struct Transform
    {
        entt::entity parent = entt::null;
        // Local
        GW::MATH::GVECTORF position{ {0.0f, 0.0f, 0.0f, 0.0f} };
        GW::MATH::GQUATERNIONF rotation{ {0.0f, 0.0f, 0.0f, 1.0f} };
        GW::MATH::GVECTORF scale{ {1.0f, 1.0f, 1.0f, 0.0f} };

        GW::MATH::GMATRIXF world{};
        bool recomputeWorld = true;

        void SetPosition(const GW::MATH::GVECTORF & newPosition) { position = newPosition; recomputeWorld = true; }
        void SetRotation(const GW::MATH::GVECTORF & newRotation) 
        { 
            rotation.x = newRotation.x;
            rotation.y = newRotation.y;
            rotation.z = newRotation.z;
            rotation.w = newRotation.w;
            recomputeWorld = true; 
        }
        void SetScale(const GW::MATH::GVECTORF & newScale) { scale = newScale; recomputeWorld = true; }
        void Translate(const GW::MATH::GVECTORF & newTranslation)
        {
            position = GW::MATH::GVECTORF
            {
                {
                    position.data[0] + newTranslation.data[0],
                    position.data[1] + newTranslation.data[1],
                    position.data[2] + newTranslation.data[2],
                    0.0f
                }
            };
            recomputeWorld = true;
        }
        void ScaleBy(const GW::MATH::GVECTORF & newScale)
        {
            scale = GW::MATH::GVECTORF
            {
                {
                    scale.data[0] * newScale.data[0],
                    scale.data[1] * newScale.data[1],
                    scale.data[2] * newScale.data[2],
                    0.0f
                }
            };
            recomputeWorld = true;
        }
        void RecalculateWorld()
        {
            GW::MATH::GMATRIXF identity = GW::MATH::GIdentityMatrixF;
            GW::MATH::GMATRIXF translate = GW::MATH::GIdentityMatrixF;
            GW::MATH::GMatrix::TranslateGlobalF(identity, position, translate);

            world = translate;

            recomputeWorld = false;
        }
    };

    struct MeshHandle { uint32_t id = UINT32_MAX; };

    struct TextureHandle { uint32_t id = UINT32_MAX; };

    struct Material
    {
        GW::MATH::GVECTORF baseColor{{{1.0f, 1.0f, 1.0f, 1.0f}}};
        TextureHandle albedoTexture;
        uint32_t shaderIndex = 0;
    };

    struct Camera
    {
        GW::MATH::GMATRIXF view{};
        GW::MATH::GMATRIXF projection{};
    };

    struct GPU_CBUFFER
    {
        GW::MATH::GMATRIXF World;
        GW::MATH::GMATRIXF View;
        GW::MATH::GMATRIXF Projection;
    };

    struct RendererComponent
    {
        GLFWwindow* window = nullptr;

        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;
        std::vector<VkImage> swapchainImages;
        std::vector<VkImageView> swapchainImageViews;
        std::vector<VkFramebuffer> swapchainFramebuffers;

        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;

        VkCommandPool commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> commandBuffers;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

        // sync
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        size_t currentFrame = 0;

        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
        VkBuffer uniformBuffer = VK_NULL_HANDLE;
        VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
        VkSampler textureSampler = VK_NULL_HANDLE;
        VkImage textureImage = VK_NULL_HANDLE;
        VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
        VkImageView textureImageView = VK_NULL_HANDLE;

        size_t indexCount = 0;
        // Depth resources
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;

        const int MAX_FRAMES_IN_FLIGHT = 2;
        bool framebufferResized = false;
        uint32_t lastImageIndex = 0u;

        GLFWwindow* GetWindow() const { return window; }
    };

    struct GeometryData
    {
        unsigned int indexStart, indexCount, vertexStart;
        inline bool operator < (const GeometryData a) const 
        {
            return indexStart < a.indexStart;
        }
    };

    struct GPUInstance
    {
        GW::MATH::GMATRIXF transform;
        Material materialData;
    };

    struct MeshCollection
    {
        std::string collectionName;
        std::vector<entt::entity> entities;

        GW::MATH::GOBBF collider;
    };

    struct ModelManager
    {
        std::vector<MeshCollection> meshCollections;
    };

    //*** CLASSES ***//
    class MeshManager
    {
    public:
        static MeshManager& Instance();

        uint32_t LoadMesh(const std::string& name, const std::string& filepath);

        uint32_t GetMeshId(const std::string& name) const;

        const MeshResource* GetMesh(uint32_t id) const;
        size_t MeshCount() const;

        void ReleaseCpuMeshData(uint32_t id);
        void ReleaseAllGpuResources(RendererComponent& renderer);

    private:
        MeshManager() = default;
        std::vector<MeshResource> resources;
        std::unordered_map<std::string, uint32_t> nameToId;
        mutable std::mutex mutex;
    };

    // *** FUNCTIONS *** //
    void InitializeGraphics(entt::registry& registry);
    void SetDebugName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name);

    // *** RENDERER HELPERS *** //
    namespace RENDERER_HELPERS
    {
        // Bind/unbind an externally owned RendererComponent
        void BindRenderer(RendererComponent* renderer);
        void UnbindRenderer();
        RendererComponent* GetBoundRenderer();

        // Per-instance initialization / lifecycle
        bool InitializeFor(RendererComponent& rendererComponent, uint32_t width, uint32_t height, const char* title);
        bool InitializeWindowFor(RendererComponent& rendererComponent, uint32_t width, uint32_t height, const char* title);
        bool InitializeVulkanFor(RendererComponent& rendererComponent);
        void CleanupFor(RendererComponent& rendererComponent);

        // Rendering APIs (per-instance)
        void RenderFor(RendererComponent& rendererComponent);
        void UpdateUniformsFor(RendererComponent& rendererComponent, const void* data, size_t size);

        // Top-level lifecycle
        bool Initialize(uint32_t width, uint32_t height, const char* title);
        void RenderingLoop();
        void Cleanup();
        void UpdateUniforms(const void* data, size_t size);
        void Render();
        RendererComponent* GetGlobalRenderer();

        // Window initialization
        bool InitializeWindow(uint32_t width, uint32_t height, const char* title); 
        void FramebufferResize(GLFWwindow* window, int width, int height);

        // Draw frame:
        void DrawFrame();
        void DrawFrameFor(RendererComponent& rendererComponent);

        // Device-level helpers
        namespace Device
        {
            struct QueueFamilyIndices
            {
                std::optional<uint32_t> graphicsFamily;
                std::optional<uint32_t> presentFamily;

                bool IsComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
            };

            QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
            bool CreateInstanceFor(RendererComponent& rendererComponent);
            bool CreateSurfaceFor(RendererComponent& rendererComponent);
            bool PickPhysicalDeviceFor(RendererComponent& rendererComponent);
            bool CreateLogicalDeviceFor(RendererComponent& rendererComponent);
        }

        // Swapchain & image views
        namespace Swapchain
        {
            bool CreateSwapchainFor(RendererComponent& rendererComponent);
            bool CreateImageViewsFor(RendererComponent& rendererComponent);
            bool CreateFramebuffersFor(RendererComponent& rendererComponent);
            void RecreateSwapchainFor(RendererComponent& rendererComponent);
        }

        namespace Depth
        {
            VkFormat FindDepthFormat(RendererComponent& rendererComponent);
            bool HasStencilComponent(VkFormat format);
            VkFormat FindDepthFormat(RendererComponent& rendererComponent);
            bool CreateDepthResourcesFor(RendererComponent& rendererComponent);
        }

        // Memory and buffer utilities
        namespace Memory
        {
            uint32_t FindMemoryTypeFor(RendererComponent& rendererComponent, uint32_t typeFilter, VkMemoryPropertyFlags properties);
            void CreateBufferFor(RendererComponent& rendererComponent, VkDeviceSize size, VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
            VkCommandBuffer BeginSingleTimeCommandsFor(RendererComponent& rendererComponent);
            void EndSingleTimeCommandsFor(RendererComponent& rendererComponent, VkCommandBuffer commandBuffer);
            void CreateVertexAndIndexBuffersFor(RendererComponent& rendererComponent, 
                std::vector<FBXVertex> vertices, std::vector<uint32_t> indices,
                VkBuffer& outVertexBuffer, VkDeviceMemory& outVertexMemory, 
                VkBuffer& outIndexBuffer, VkDeviceMemory& outIndexMemory, size_t& outIndexCount);
        }

        // Image / texture utilities
        namespace Image
        {
            void CreateImageFor(RendererComponent& rendererComponent, uint32_t width, uint32_t height, VkFormat format,
                                VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                                VkImage& image, VkDeviceMemory& imageMemory);
            VkImageView CreateImageViewForTextureFor(RendererComponent& rendererComponent, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
            void TransitionImageLayoutFor(RendererComponent& rendererComponent, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
            void CopyBufferToImageFor(RendererComponent& rendererComponent, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
            bool CreateTextureFromPixelsFor(RendererComponent& rendererComponent, const void* pixels, uint32_t textureWidth, uint32_t textureHeight, VkFormat format);
        }

        // Pipeline, render pass, shaders
        namespace Pipeline
        {
            bool CreateRenderPassFor(RendererComponent& rendererComponent);
            bool CreateGraphicsPipelineFor(RendererComponent& rendererComponent);
            VkShaderModule CreateShaderModuleFor(RendererComponent& rendererComponent, const std::vector<char>& code);
            bool CreateCommandBuffersFor(RendererComponent& rendererComponent);
            bool CreateCommandPoolFor(RendererComponent& rendererComponent);
            bool CreateSyncObjectsFor(RendererComponent& rendererComponent);
        }

        // Small utilities
        namespace Utilities
        {
            std::vector<char> ReadFile(const std::string& filename);
            std::vector<char> ReadFileBinary(const std::string& path);
            bool CompileHLSLWithDXC(const std::string& dxcPath, const std::string& hlsl, const std::string& outSpv, const std::string& profile);
            std::vector<char> LoadOrCompileShader(const std::string& hlslPath, const std::string& spvPath, const std::string& profile, const std::string& dxcPath = "dxc");
        }
    } // namespace RENDERER_HELPERS

    // *** CAMERA_SYSTEM *** //
    namespace CAMERA_SYSTEM
    {
        entt::entity CreateCamera(entt::registry& registry, float fovRadians, float aspect, float nearZ, float farZ);
        void UpdateCameraAndUpload(entt::registry& registry, entt::entity cameraEntity);
    }

    // *** Using declarations *** //
    using namespace RENDERING;
    using namespace RENDERING::RENDERER_HELPERS;
    using namespace RENDERING::RENDERER_HELPERS::Memory;
    using namespace RENDERING::RENDERER_HELPERS::Depth;
    using namespace RENDERING::RENDERER_HELPERS::Memory;
    using namespace RENDERING::RENDERER_HELPERS::Pipeline;
    using namespace RENDERING::RENDERER_HELPERS::Swapchain;
    using namespace RENDERING::RENDERER_HELPERS::Device;
    using namespace RENDERING::RENDERER_HELPERS::Image;

    using Registry = entt::registry;
} // namespace RENDERING