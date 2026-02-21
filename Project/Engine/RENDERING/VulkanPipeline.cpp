#include "RenderingComponents.h"
#include <array>
#include <cstring>

namespace RENDERING::RENDERER_HELPERS::Pipeline
{
    bool CreateRenderPassFor(RendererComponent& rendererComponent)
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = rendererComponent.swapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Query a supported depth format instead of hardcoding
        VkFormat depthFormat = RENDERING::RENDERER_HELPERS::Depth::FindDepthFormat(rendererComponent);

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentReference{};
        depthAttachmentReference.attachment = 1;
        depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentReference;
        subpass.pDepthStencilAttachment = &depthAttachmentReference;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        // Add subpass dependency to handle layout transitions and synchronization between render pass and engine
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(rendererComponent.device, &renderPassInfo, nullptr, &rendererComponent.renderPass) != VK_SUCCESS)
        {
            printf("Failed to create render pass\n");
            return false;
        }

        return true;
    }

    bool CreateGraphicsPipelineFor(RendererComponent& rendererComponent)
    {
        // Load or compile shaders
        std::string vertexHlsl = "../../Shaders/VertexShader.hlsl";
        std::string fragmentHlsl = "../../Shaders/PixelShader.hlsl";
        std::string vertexSpv = "shaders/VertexShader.spv";
        std::string fragmentSpv = "shaders/PixelShader.spv";

        std::vector<char> vertexShaderCode = RENDERING::RENDERER_HELPERS::Utilities::LoadOrCompileShader(vertexHlsl, vertexSpv, "vs_6_0");
        std::vector<char> fragmentShaderCode = RENDERING::RENDERER_HELPERS::Utilities::LoadOrCompileShader(fragmentHlsl, fragmentSpv, "ps_6_0");

        // fallback to existing SPV if runtime compilation failed
        if (vertexShaderCode.empty()) vertexShaderCode = RENDERING::RENDERER_HELPERS::Utilities::ReadFile(vertexSpv);
        if (fragmentShaderCode.empty()) fragmentShaderCode = RENDERING::RENDERER_HELPERS::Utilities::ReadFile(fragmentSpv);

        if (vertexShaderCode.empty() || fragmentShaderCode.empty())
        {
            printf("Failed to load or compile shaders\n");
            return false;
        }

        // create shader modules using helper
        VkShaderModule vertexShaderModule = RENDERING::RENDERER_HELPERS::Pipeline::CreateShaderModuleFor(rendererComponent, vertexShaderCode);
        VkShaderModule fragmentShaderModule = RENDERING::RENDERER_HELPERS::Pipeline::CreateShaderModuleFor(rendererComponent, fragmentShaderCode);

        VkPipelineShaderStageCreateInfo vertexStageInfo{};
        vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexStageInfo.module = vertexShaderModule;
        vertexStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentStageInfo{};
        fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentStageInfo.module = fragmentShaderModule;
        fragmentStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertexStageInfo, fragmentStageInfo };

        // Vertex input binding
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(FBXVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // Attribute descriptions: location 0 = position (vec3), 1 = texcoord (vec2), 2 = normal (vec3)
        VkVertexInputAttributeDescription attributeDescriptions[3];

        // location 0: position (vec3)
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(FBXVertex, position);

        // location 1: texcoord (vec2)
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(FBXVertex, uv);

        // location 2: normal (vec3)
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(FBXVertex, normal);

        // Fill VkPipelineVertexInputStateCreateInfo
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = 3;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)rendererComponent.swapchainExtent.width;
        viewport.height = (float)rendererComponent.swapchainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = rendererComponent.swapchainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        // descriptor bindings
        VkDescriptorSetLayoutBinding uboBinding0{};
        uboBinding0.binding = 0;
        uboBinding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding0.descriptorCount = 1;
        uboBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        uboBinding0.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding uboBinding1{};
        uboBinding1.binding = 1;
        uboBinding1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding1.descriptorCount = 1;
        uboBinding1.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboBinding1.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 2;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        samplerBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboBinding0, uboBinding1, samplerBinding };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout descriptorSetLayout;
        VkResult result = vkCreateDescriptorSetLayout(rendererComponent.device, &layoutInfo, nullptr, &descriptorSetLayout);
        if (result != VK_SUCCESS)
        {
            printf("vkCreateDescriptorSetLayout failed: %d\n", result);
            vkDestroyShaderModule(rendererComponent.device, fragmentShaderModule, nullptr);
            vkDestroyShaderModule(rendererComponent.device, vertexShaderModule, nullptr);
            return false;
        }

        // Descriptor pool and set allocation
        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 1;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = 1;
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[2].descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1;

        result = vkCreateDescriptorPool(rendererComponent.device, &poolInfo, nullptr, &rendererComponent.descriptorPool);
        if (result != VK_SUCCESS)
        {
            printf("vkCreateDescriptorPool failed: %d\n", result);
            vkDestroyDescriptorSetLayout(rendererComponent.device, descriptorSetLayout, nullptr);
            vkDestroyShaderModule(rendererComponent.device, fragmentShaderModule, nullptr);
            vkDestroyShaderModule(rendererComponent.device, vertexShaderModule, nullptr);
            return false;
        }

        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = rendererComponent.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &descriptorSetLayout;

        result = vkAllocateDescriptorSets(rendererComponent.device, &allocateInfo, &rendererComponent.descriptorSet);
        if (result != VK_SUCCESS)
        {
            printf("vkAllocateDescriptorSets failed: %d\n", result);
            vkDestroyDescriptorPool(rendererComponent.device, rendererComponent.descriptorPool, nullptr);
            rendererComponent.descriptorPool = VK_NULL_HANDLE;
            vkDestroyDescriptorSetLayout(rendererComponent.device, descriptorSetLayout, nullptr);
            vkDestroyShaderModule(rendererComponent.device, fragmentShaderModule, nullptr);
            vkDestroyShaderModule(rendererComponent.device, vertexShaderModule, nullptr);
            return false;
        }

        // Create uniform buffer
        {
            VkDeviceSize uboSize = sizeof(RENDERING::GPU_CBUFFER);
            try
            {
                RENDERING::RENDERER_HELPERS::Memory::CreateBufferFor(rendererComponent, uboSize,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    rendererComponent.uniformBuffer,
                    rendererComponent.uniformBufferMemory);
            }
            catch (const std::exception& exception)
            {
                printf("Failed to create uniform buffer: %s\n", exception.what());
                vkDestroyDescriptorPool(rendererComponent.device, rendererComponent.descriptorPool, nullptr);
                rendererComponent.descriptorPool = VK_NULL_HANDLE;
                vkDestroyDescriptorSetLayout(rendererComponent.device, descriptorSetLayout, nullptr);
                vkDestroyShaderModule(rendererComponent.device, fragmentShaderModule, nullptr);
                vkDestroyShaderModule(rendererComponent.device, vertexShaderModule, nullptr);
                return false;
            }
        }

        // create textureSampler
        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 0.0f;

            if (vkCreateSampler(rendererComponent.device, &samplerInfo, nullptr, &rendererComponent.textureSampler) != VK_SUCCESS)
            {
                printf("Failed to create texture sampler\n");
                vkDestroyDescriptorSetLayout(rendererComponent.device, descriptorSetLayout, nullptr);
                vkDestroyShaderModule(rendererComponent.device, fragmentShaderModule, nullptr);
                vkDestroyShaderModule(rendererComponent.device, vertexShaderModule, nullptr);
                return false;
            }
        }

        // Create a simple 1x1 white texture for the default
        if (rendererComponent.textureImageView == VK_NULL_HANDLE)
        {
            unsigned char whitePixel[4] = { 255u, 255u, 255u, 255u };
            RENDERING::RENDERER_HELPERS::Image::CreateTextureFromPixelsFor(rendererComponent, whitePixel, 1u, 1u, VK_FORMAT_R8G8B8A8_UNORM);
        }

        // Update descriptor sets
        {
            VkDeviceSize uboSize = sizeof(RENDERING::GPU_CBUFFER);

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = rendererComponent.uniformBuffer;
            bufferInfo.offset = 0;
            bufferInfo.range = uboSize;

            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = rendererComponent.textureSampler;
            imageInfo.imageView = rendererComponent.textureImageView;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

            // binding 0 -> bufferInfo0
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = rendererComponent.descriptorSet;
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            // binding 1 -> bufferInfo1
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = rendererComponent.descriptorSet;
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &bufferInfo;

            // binding 2 -> imageInfo
            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = rendererComponent.descriptorSet;
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(rendererComponent.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

        VkPushConstantRange pushConstRange{};
        pushConstRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstRange.offset = 0;
        pushConstRange.size = sizeof(float) * 16; // model matrix (4x4 floats)

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstRange;

        result = vkCreatePipelineLayout(rendererComponent.device, &pipelineLayoutInfo, nullptr, &rendererComponent.pipelineLayout);
        if (result != VK_SUCCESS)
        {
            printf("vkCreatePipelineLayout failed: %d\n", result);
            if (rendererComponent.descriptorPool) { vkDestroyDescriptorPool(rendererComponent.device, rendererComponent.descriptorPool, nullptr); rendererComponent.descriptorPool = VK_NULL_HANDLE; }
            vkDestroyDescriptorSetLayout(rendererComponent.device, descriptorSetLayout, nullptr);
            vkDestroyShaderModule(rendererComponent.device, fragmentShaderModule, nullptr);
            vkDestroyShaderModule(rendererComponent.device, vertexShaderModule, nullptr);
            return false;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.layout = rendererComponent.pipelineLayout;
        pipelineInfo.renderPass = rendererComponent.renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(rendererComponent.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &rendererComponent.graphicsPipeline) != VK_SUCCESS)
        {
            printf("Failed to create graphics pipeline\n");
            if (rendererComponent.descriptorPool) { vkDestroyDescriptorPool(rendererComponent.device, rendererComponent.descriptorPool, nullptr); rendererComponent.descriptorPool = VK_NULL_HANDLE; }
            vkDestroyPipelineLayout(rendererComponent.device, rendererComponent.pipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(rendererComponent.device, descriptorSetLayout, nullptr);
            vkDestroyShaderModule(rendererComponent.device, fragmentShaderModule, nullptr);
            vkDestroyShaderModule(rendererComponent.device, vertexShaderModule, nullptr);
            return false;
        }

        vkDestroyDescriptorSetLayout(rendererComponent.device, descriptorSetLayout, nullptr);
        vkDestroyShaderModule(rendererComponent.device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(rendererComponent.device, vertexShaderModule, nullptr);

        return true;
    }

    bool CreateCommandBuffersFor(RendererComponent& rendererComponent)
    {
        // if command buffers aren't full, empty them for re-allocation
        if (!rendererComponent.commandBuffers.empty() && rendererComponent.commandPool != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(rendererComponent.device,
                rendererComponent.commandPool,
                static_cast<uint32_t>(rendererComponent.commandBuffers.size()),
                rendererComponent.commandBuffers.data());
            rendererComponent.commandBuffers.clear();
        }

        rendererComponent.commandBuffers.resize(rendererComponent.swapchainFramebuffers.size());

        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = rendererComponent.commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = (uint32_t)rendererComponent.commandBuffers.size();

        if (vkAllocateCommandBuffers(rendererComponent.device, &allocateInfo, rendererComponent.commandBuffers.data()) != VK_SUCCESS)
        {
            printf("Failed to allocate command buffers\n");
            return false;
        }
        return true;
    }

    bool CreateCommandPoolFor(RendererComponent& rendererComponent)
    {
        QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(rendererComponent.physicalDevice, rendererComponent.surface);
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(rendererComponent.device, &poolInfo, nullptr, &rendererComponent.commandPool) != VK_SUCCESS)
        {
            printf("Failed to create command pool\n");
            return false;
        }

        return true;
    }

    bool CreateSyncObjectsFor(RendererComponent& rendererComponent)
    {
        // imageAvailable semaphores: per-frame
        rendererComponent.imageAvailableSemaphores.resize(rendererComponent.MAX_FRAMES_IN_FLIGHT);
        // renderFinished semaphores: per-swapchain-image
        rendererComponent.renderFinishedSemaphores.resize(rendererComponent.swapchainImages.size());
        // fences: per-frame
        rendererComponent.inFlightFences.resize(rendererComponent.MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        // Create per-frame semaphores + fences
        for (size_t frame = 0; frame < rendererComponent.MAX_FRAMES_IN_FLIGHT; ++frame)
        {
            if (vkCreateSemaphore(rendererComponent.device, &semaphoreInfo, nullptr, &rendererComponent.imageAvailableSemaphores[frame]) != VK_SUCCESS ||
                vkCreateFence(rendererComponent.device, &fenceInfo, nullptr, &rendererComponent.inFlightFences[frame]) != VK_SUCCESS)
            {
                printf("Failed to create per-frame synchronization objects\n");
                return false;
            }
        }

        // Create per-swapchain-image render-finished semaphores
        for (size_t semaphore = 0; semaphore < rendererComponent.renderFinishedSemaphores.size(); ++semaphore)
        {
            if (vkCreateSemaphore(rendererComponent.device, &semaphoreInfo, nullptr, &rendererComponent.renderFinishedSemaphores[semaphore]) != VK_SUCCESS)
            {
                printf("Failed to create render-finished semaphore for image %zu\n", semaphore);
                return false;
            }
        }

        return true;
    }
}