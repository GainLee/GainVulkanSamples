/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Gain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Sample_10_PBR.h"

#define GLM_FORCE_RADIANS

#include "VulkanglTFModel.h"

void Sample_10_PBR::set3DModelPath(std::string path)
{
    mModelPath = path;
}

void Sample_10_PBR::prepareSynchronizationPrimitives()
{
    // Semaphores (Used for correct command ordering)
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext                 = nullptr;

    // Semaphore used to ensures that image presentation is complete before starting to submit again
    CALL_VK(vkCreateSemaphore(
        device(), &semaphoreCreateInfo, nullptr, presentCompleteSemaphore.pHandle()));

    // Semaphore used to ensures that all commands submitted have been finished before submitting
    // the image to the queue
    CALL_VK(vkCreateSemaphore(
        device(), &semaphoreCreateInfo, nullptr, renderCompleteSemaphore.pHandle()));
}

void Sample_10_PBR::updateUniformBuffers()
{
    // Scene
    shaderValuesScene.projection = mCamera.matrices.perspective;
    shaderValuesScene.view       = mCamera.matrices.view;

    // Center and scale model
    float     scale     = (1.0f / std::max(pbrModels.scene.aabb[0][0], std::max(pbrModels.scene.aabb[1][1], pbrModels.scene.aabb[2][2]))) * 0.5f;
    glm::vec3 translate = -glm::vec3(pbrModels.scene.aabb[3][0], pbrModels.scene.aabb[3][1], pbrModels.scene.aabb[3][2]);
    translate += -0.5f * glm::vec3(pbrModels.scene.aabb[0][0], pbrModels.scene.aabb[1][1], pbrModels.scene.aabb[2][2]);

    shaderValuesScene.model       = glm::mat4(1.0f);
    shaderValuesScene.model[0][0] = scale;
    shaderValuesScene.model[1][1] = scale;
    shaderValuesScene.model[2][2] = scale;
    shaderValuesScene.model       = glm::translate(shaderValuesScene.model, translate);

    shaderValuesScene.camPos = glm::vec3(
        -mCamera.position.z * sin(glm::radians(mCamera.rotation.y)) * cos(glm::radians(mCamera.rotation.x)),
        -mCamera.position.z * sin(glm::radians(mCamera.rotation.x)),
        mCamera.position.z * cos(glm::radians(mCamera.rotation.y)) * cos(glm::radians(mCamera.rotation.x)));

    // Skybox
    shaderValuesSkybox.projection = mCamera.matrices.perspective;
    shaderValuesSkybox.view       = mCamera.matrices.view;
    shaderValuesSkybox.model      = glm::mat4(glm::mat3(mCamera.matrices.view));

    for (auto &uniformBuffer : uniformBuffers)
    {
        uniformBuffer.scene->copyFrom(&shaderValuesScene);
        uniformBuffer.skybox->copyFrom(&shaderValuesSkybox);
        uniformBuffer.params->copyFrom(&shaderValuesParams);
    }
}

void Sample_10_PBR::setupDescriptorSetLayout()
{
    descriptorSetLayouts.scene    = VulkanDescriptorSetLayout(device());
    descriptorSetLayouts.material = VulkanDescriptorSetLayout(device());
    descriptorSetLayouts.node     = VulkanDescriptorSetLayout(device());

    // Scene (matrices and environment maps)
    {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI =
            vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
        CALL_VK(vkCreateDescriptorSetLayout(
            device(), &descriptorSetLayoutCI, nullptr, descriptorSetLayouts.scene.pHandle()));
        vks::debug::setDescriptorSetLayoutName(device(), descriptorSetLayouts.scene.handle(), "descriptorSetLayouts.scene");
    }

    // Material (samplers)
    {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI =
            vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
        CALL_VK(vkCreateDescriptorSetLayout(
            device(), &descriptorSetLayoutCI, nullptr, descriptorSetLayouts.material.pHandle()));
        vks::debug::setDescriptorSetLayoutName(device(), descriptorSetLayouts.material.handle(), "descriptorSetLayouts.material");
    }

    // Descriptor set layout for nodes
    {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}};
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
        CALL_VK(vkCreateDescriptorSetLayout(device(), &descriptorSetLayoutCI, nullptr, descriptorSetLayouts.node.pHandle()));
        vks::debug::setDescriptorSetLayoutName(device(), descriptorSetLayouts.node.handle(), "descriptorSetLayouts.node");
    }

    // Pipeline layout using descriptor sets (set 0 = scene, set 1 = material, set 2 = node)
    std::vector<VkDescriptorSetLayout> setLayouts = {
        descriptorSetLayouts.scene.handle(),
        descriptorSetLayouts.material.handle(),
        descriptorSetLayouts.node.handle()};

    VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(
        setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.size                  = sizeof(PushConstBlockMaterial);
    pushConstantRange.stageFlags            = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineLayoutCI.pushConstantRangeCount = 1;
    pipelineLayoutCI.pPushConstantRanges    = &pushConstantRange;

    mPipelineLayout = VulkanPipelineLayout(device());
    CALL_VK(
        vkCreatePipelineLayout(device(), &pipelineLayoutCI, nullptr, mPipelineLayout.pHandle()));
}

void Sample_10_PBR::setupDescriptorPool()
{
    uint32_t imageSamplerCount = 0;
    uint32_t materialCount     = 0;
    uint32_t meshCount         = 0;

    // Environment samplers (radiance, irradiance, brdf lut)
    imageSamplerCount += 3;

    std::vector<vkglTF::Model *> modellist = {&pbrModels.skybox, &pbrModels.scene};
    for (auto &model : modellist)
    {
        for (auto &material : model->materials)
        {
            imageSamplerCount += 5;
            materialCount++;
        }
        for (auto node : model->linearNodes)
        {
            if (node->mesh)
            {
                meshCount++;
            }
        }
    }

    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (4 + meshCount) * mSwapChain.imageCount),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                              imageSamplerCount * mSwapChain.imageCount),
    };

    const uint32_t             maxSetCount = (2 + materialCount + meshCount) * mSwapChain.imageCount;
    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vks::initializers::descriptorPoolCreateInfo(poolSizes, maxSetCount);
    CALL_VK(
        vkCreateDescriptorPool(device(), &descriptorPoolInfo, nullptr, mDescriptorPool.pHandle()));
}

void Sample_10_PBR::prepare(JNIEnv *env)
{
    if (!mPrepared)
    {
        VulkanContextBase::prepare(env);

        prepare3DModel(env);

        initCameraView();

        prepareSynchronizationPrimitives();
        prepareUniformBuffers();
        setupDescriptorPool();
        setupDescriptorSetLayout();
        setupDescriptorSet();
        preparePipelines();
        buildCommandBuffers();

        mPrepared = true;
    }

    updateUniformBuffers();
}

void Sample_10_PBR::initCameraView()
{
    mCamera.type          = Camera::CameraType::lookat;
    mCamera.rotationSpeed = 0.25f;
    mCamera.movementSpeed = 0.1f;
    mCamera.setPosition({0.0f, 0.0f, 2.0f});
    mCamera.setRotation({0.0f, 0.0f, 0.0f});
}

void Sample_10_PBR::prepare3DModel(JNIEnv *env)
{
    vkglTF::setupAssetManager(mAssetManager);
    pbrModels.scene.destroy(device());
    pbrModels.scene.loadFromFile(mModelPath, deviceWrapper(), mGraphicsQueue);

    pbrModels.skybox.loadFromFile("models/Box/glTF-Embedded/Box.gltf", deviceWrapper(), mGraphicsQueue);

    std::string           envMapFile = "environments/papermill.ktx";
    Image::ImageBasicInfo imageInfo  = {format: VK_FORMAT_R16G16B16A16_SFLOAT,
                                       usage: VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT};
    textures.environmentCube         = gain::Image::createCubeMapFromFile(deviceWrapper(), mGraphicsQueue, mAssetManager, envMapFile, imageInfo);
    generateCubemaps();
    generateBRDFLUT();
}

void Sample_10_PBR::generateCubemaps()
{
    enum Target
    {
        IRRADIANCE     = 0,
        PREFILTEREDENV = 1
    };

    for (uint32_t target = 0; target < PREFILTEREDENV + 1; target++)
    {
        std::unique_ptr<Image> cubemap;

        auto tStart = std::chrono::high_resolution_clock::now();

        VkFormat format;
        int32_t  dim;

        switch (target)
        {
            case IRRADIANCE:
                format = VK_FORMAT_R32G32B32A32_SFLOAT;
                dim    = 64;
                break;
            case PREFILTEREDENV:
                format = VK_FORMAT_R16G16B16A16_SFLOAT;
                dim    = 512;
                break;
        };

        const uint32_t numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;

        // Create target cubemap
        {
            // Image
            gain::Image::ImageBasicInfo imageInfo = {
                format: format,
                imageType: VK_IMAGE_TYPE_2D,
                mipLevels: numMips,
                arrayLayers: 6,
                usage: VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                layout: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            imageInfo.extent.width  = dim;
            imageInfo.extent.height = dim;
            imageInfo.extent.depth  = 1;
            cubemap                 = gain::Image::createDeviceLocal(deviceWrapper(), mGraphicsQueue, imageInfo);
            vks::debug::setSamplerName(device(), cubemap->getSamplerHandle(), "cube_sampler");
        }

        // FB, Att, RP, Pipe, etc.
        VkAttachmentDescription attDesc{};
        // Color attachment
        attDesc.format                       = format;
        attDesc.samples                      = VK_SAMPLE_COUNT_1_BIT;
        attDesc.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
        attDesc.stencilLoadOp                = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc.stencilStoreOp               = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.initialLayout                = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout                  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments    = &colorReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass      = 0;
        dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass      = 0;
        dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Renderpass
        VkRenderPassCreateInfo renderPassCI{};
        renderPassCI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCI.attachmentCount = 1;
        renderPassCI.pAttachments    = &attDesc;
        renderPassCI.subpassCount    = 1;
        renderPassCI.pSubpasses      = &subpassDescription;
        renderPassCI.dependencyCount = 2;
        renderPassCI.pDependencies   = dependencies.data();
        VkRenderPass renderpass;
        CALL_VK(vkCreateRenderPass(deviceWrapper()->logicalDevice, &renderPassCI, nullptr, &renderpass));

        struct Offscreen
        {
            VkImage        image;
            VkImageView    view;
            VkDeviceMemory memory;
            VkFramebuffer  framebuffer;
        } offscreen;

        // Create offscreen framebuffer
        {
            // Image
            VkImageCreateInfo imageCI{};
            imageCI.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCI.imageType     = VK_IMAGE_TYPE_2D;
            imageCI.format        = format;
            imageCI.extent.width  = dim;
            imageCI.extent.height = dim;
            imageCI.extent.depth  = 1;
            imageCI.mipLevels     = 1;
            imageCI.arrayLayers   = 1;
            imageCI.samples       = VK_SAMPLE_COUNT_1_BIT;
            imageCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCI.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            imageCI.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            CALL_VK(vkCreateImage(deviceWrapper()->logicalDevice, &imageCI, nullptr, &offscreen.image));
            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(deviceWrapper()->logicalDevice, offscreen.image, &memReqs);
            VkMemoryAllocateInfo memAllocInfo{};
            memAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memAllocInfo.allocationSize  = memReqs.size;
            memAllocInfo.memoryTypeIndex = deviceWrapper()->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            CALL_VK(vkAllocateMemory(deviceWrapper()->logicalDevice, &memAllocInfo, nullptr, &offscreen.memory));
            CALL_VK(vkBindImageMemory(deviceWrapper()->logicalDevice, offscreen.image, offscreen.memory, 0));

            // View
            VkImageViewCreateInfo viewCI{};
            viewCI.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            viewCI.format                          = format;
            viewCI.flags                           = 0;
            viewCI.subresourceRange                = {};
            viewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            viewCI.subresourceRange.baseMipLevel   = 0;
            viewCI.subresourceRange.levelCount     = 1;
            viewCI.subresourceRange.baseArrayLayer = 0;
            viewCI.subresourceRange.layerCount     = 1;
            viewCI.image                           = offscreen.image;
            CALL_VK(vkCreateImageView(deviceWrapper()->logicalDevice, &viewCI, nullptr, &offscreen.view));

            // Framebuffer
            VkFramebufferCreateInfo framebufferCI{};
            framebufferCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCI.renderPass      = renderpass;
            framebufferCI.attachmentCount = 1;
            framebufferCI.pAttachments    = &offscreen.view;
            framebufferCI.width           = dim;
            framebufferCI.height          = dim;
            framebufferCI.layers          = 1;
            CALL_VK(vkCreateFramebuffer(deviceWrapper()->logicalDevice, &framebufferCI, nullptr, &offscreen.framebuffer));

            VkCommandBuffer      layoutCmd = deviceWrapper()->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.image            = offscreen.image;
            imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageMemoryBarrier.srcAccessMask    = 0;
            imageMemoryBarrier.dstAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            vkCmdPipelineBarrier(layoutCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            deviceWrapper()->endAndSubmitSingleTimeCommand(layoutCmd, mGraphicsQueue, true);
        }

        // Descriptors
        VkDescriptorSetLayout           descriptorsetlayout;
        VkDescriptorSetLayoutBinding    setLayoutBinding = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
        descriptorSetLayoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCI.pBindings    = &setLayoutBinding;
        descriptorSetLayoutCI.bindingCount = 1;
        CALL_VK(vkCreateDescriptorSetLayout(deviceWrapper()->logicalDevice, &descriptorSetLayoutCI, nullptr, &descriptorsetlayout));

        // Descriptor Pool
        VkDescriptorPoolSize       poolSize = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};
        VkDescriptorPoolCreateInfo descriptorPoolCI{};
        descriptorPoolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCI.poolSizeCount = 1;
        descriptorPoolCI.pPoolSizes    = &poolSize;
        descriptorPoolCI.maxSets       = 2;
        VkDescriptorPool descriptorpool;
        CALL_VK(vkCreateDescriptorPool(deviceWrapper()->logicalDevice, &descriptorPoolCI, nullptr, &descriptorpool));

        // Descriptor sets
        VkDescriptorSet             descriptorset;
        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool     = descriptorpool;
        descriptorSetAllocInfo.pSetLayouts        = &descriptorsetlayout;
        descriptorSetAllocInfo.descriptorSetCount = 1;
        CALL_VK(vkAllocateDescriptorSets(deviceWrapper()->logicalDevice, &descriptorSetAllocInfo, &descriptorset));
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstSet          = descriptorset;
        writeDescriptorSet.dstBinding      = 0;
        auto desc                          = textures.environmentCube->getDescriptor();
        writeDescriptorSet.pImageInfo      = &desc;
        vkUpdateDescriptorSets(deviceWrapper()->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);

        struct PushBlockIrradiance
        {
            glm::mat4 mvp;
            float     deltaPhi   = (2.0f * float(M_PI)) / 180.0f;
            float     deltaTheta = (0.5f * float(M_PI)) / 64.0f;
        } pushBlockIrradiance;

        struct PushBlockPrefilterEnv
        {
            glm::mat4 mvp;
            float     roughness;
            uint32_t  numSamples = 32u;
        } pushBlockPrefilterEnv;

        // Pipeline layout
        VkPipelineLayout    pipelinelayout;
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        switch (target)
        {
            case IRRADIANCE:
                pushConstantRange.size = sizeof(PushBlockIrradiance);
                break;
            case PREFILTEREDENV:
                pushConstantRange.size = sizeof(PushBlockPrefilterEnv);
                break;
        };

        VkPipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.setLayoutCount         = 1;
        pipelineLayoutCI.pSetLayouts            = &descriptorsetlayout;
        pipelineLayoutCI.pushConstantRangeCount = 1;
        pipelineLayoutCI.pPushConstantRanges    = &pushConstantRange;
        CALL_VK(vkCreatePipelineLayout(deviceWrapper()->logicalDevice, &pipelineLayoutCI, nullptr, &pipelinelayout));

        // Pipeline
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
        inputAssemblyStateCI.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
        rasterizationStateCI.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCI.cullMode    = VK_CULL_MODE_NONE;
        rasterizationStateCI.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCI.lineWidth   = 1.0f;

        VkPipelineColorBlendAttachmentState blendAttachmentState{};
        blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachmentState.blendEnable    = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
        colorBlendStateCI.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCI.attachmentCount = 1;
        colorBlendStateCI.pAttachments    = &blendAttachmentState;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
        depthStencilStateCI.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCI.depthTestEnable  = VK_FALSE;
        depthStencilStateCI.depthWriteEnable = VK_FALSE;
        depthStencilStateCI.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencilStateCI.front            = depthStencilStateCI.back;
        depthStencilStateCI.back.compareOp   = VK_COMPARE_OP_ALWAYS;

        VkPipelineViewportStateCreateInfo viewportStateCI{};
        viewportStateCI.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCI.viewportCount = 1;
        viewportStateCI.scissorCount  = 1;

        VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
        multisampleStateCI.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        std::vector<VkDynamicState>      dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicStateCI{};
        dynamicStateCI.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCI.pDynamicStates    = dynamicStateEnables.data();
        dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

        // Vertex input state
        VkVertexInputBindingDescription   vertexInputBinding   = {0, sizeof(vkglTF::Model::Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription vertexInputAttribute = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};

        VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
        vertexInputStateCI.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCI.vertexBindingDescriptionCount   = 1;
        vertexInputStateCI.pVertexBindingDescriptions      = &vertexInputBinding;
        vertexInputStateCI.vertexAttributeDescriptionCount = 1;
        vertexInputStateCI.pVertexAttributeDescriptions    = &vertexInputAttribute;

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

        VkGraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.layout              = pipelinelayout;
        pipelineCI.renderPass          = renderpass;
        pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
        pipelineCI.pVertexInputState   = &vertexInputStateCI;
        pipelineCI.pRasterizationState = &rasterizationStateCI;
        pipelineCI.pColorBlendState    = &colorBlendStateCI;
        pipelineCI.pMultisampleState   = &multisampleStateCI;
        pipelineCI.pViewportState      = &viewportStateCI;
        pipelineCI.pDepthStencilState  = &depthStencilStateCI;
        pipelineCI.pDynamicState       = &dynamicStateCI;
        pipelineCI.stageCount          = 2;
        pipelineCI.pStages             = shaderStages.data();
        pipelineCI.renderPass          = renderpass;

        shaderStages[0] = loadShader("shaders/base/filtercube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        switch (target)
        {
            case IRRADIANCE:
                shaderStages[1] = loadShader("shaders/base/irradiancecube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
                break;
            case PREFILTEREDENV:
                shaderStages[1] = loadShader("shaders/base/prefilterenvmap.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
                break;
        };
        VkPipeline pipeline;
        CALL_VK(vkCreateGraphicsPipelines(device(), mPipelineCache.handle(), 1, &pipelineCI, nullptr, &pipeline));
        vks::debug::setPipelineName(device(), pipeline, "generateCube_pipeline");
        for (auto shaderStage : shaderStages)
        {
            vkDestroyShaderModule(device(), shaderStage.module, nullptr);
        }

        // Render cubemap
        VkClearValue clearValues[1];
        clearValues[0].color = {{0.0f, 0.0f, 0.2f, 0.0f}};

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass               = renderpass;
        renderPassBeginInfo.framebuffer              = offscreen.framebuffer;
        renderPassBeginInfo.renderArea.extent.width  = dim;
        renderPassBeginInfo.renderArea.extent.height = dim;
        renderPassBeginInfo.clearValueCount          = 1;
        renderPassBeginInfo.pClearValues             = clearValues;

        std::vector<glm::mat4> matrices = {
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        };

        VkCommandBuffer cmdBuf = mDeviceWrapper->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);

        VkViewport viewport{};
        viewport.width    = (float) dim;
        viewport.height   = (float) dim;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.extent.width  = dim;
        scissor.extent.height = dim;

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount   = numMips;
        subresourceRange.layerCount   = 6;

        // Change image layout for all cubemap faces to transfer destination
        {
            mDeviceWrapper->beginCommandBuffer(cmdBuf);
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.image            = cubemap->getImageHandle();
            imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.srcAccessMask    = 0;
            imageMemoryBarrier.dstAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            mDeviceWrapper->endAndSubmitSingleTimeCommand(cmdBuf, mGraphicsQueue, false);
        }

        for (uint32_t m = 0; m < numMips; m++)
        {
            for (uint32_t f = 0; f < 6; f++)
            {
                mDeviceWrapper->beginCommandBuffer(cmdBuf);

                viewport.width  = static_cast<float>(dim * std::pow(0.5f, m));
                viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
                vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
                vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

                // Render scene from cube face's point of view
                vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                // Pass parameters for current pass using a push constant block
                switch (target)
                {
                    case IRRADIANCE:
                        pushBlockIrradiance.mvp = glm::perspective((float) (M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];
                        vkCmdPushConstants(cmdBuf, pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockIrradiance), &pushBlockIrradiance);
                        break;
                    case PREFILTEREDENV:
                        pushBlockPrefilterEnv.mvp       = glm::perspective((float) (M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];
                        pushBlockPrefilterEnv.roughness = (float) m / (float) (numMips - 1);
                        vkCmdPushConstants(cmdBuf, pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockPrefilterEnv), &pushBlockPrefilterEnv);
                        break;
                };

                vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 0, 1, &descriptorset, 0, NULL);

                VkDeviceSize offsets[1] = {0};

                pbrModels.skybox.draw(cmdBuf);

                vkCmdEndRenderPass(cmdBuf);

                VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
                subresourceRange.baseMipLevel            = 0;
                subresourceRange.levelCount              = numMips;
                subresourceRange.layerCount              = 6;

                {
                    VkImageMemoryBarrier imageMemoryBarrier{};
                    imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageMemoryBarrier.image            = offscreen.image;
                    imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    imageMemoryBarrier.srcAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    imageMemoryBarrier.dstAccessMask    = VK_ACCESS_TRANSFER_READ_BIT;
                    imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                }

                // Copy region for transfer from framebuffer to cube face
                VkImageCopy copyRegion{};

                copyRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.srcSubresource.baseArrayLayer = 0;
                copyRegion.srcSubresource.mipLevel       = 0;
                copyRegion.srcSubresource.layerCount     = 1;
                copyRegion.srcOffset                     = {0, 0, 0};

                copyRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.dstSubresource.baseArrayLayer = f;
                copyRegion.dstSubresource.mipLevel       = m;
                copyRegion.dstSubresource.layerCount     = 1;
                copyRegion.dstOffset                     = {0, 0, 0};

                copyRegion.extent.width  = static_cast<uint32_t>(viewport.width);
                copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
                copyRegion.extent.depth  = 1;

                vkCmdCopyImage(
                    cmdBuf,
                    offscreen.image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    cubemap->getImageHandle(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &copyRegion);

                {
                    VkImageMemoryBarrier imageMemoryBarrier{};
                    imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageMemoryBarrier.image            = offscreen.image;
                    imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    imageMemoryBarrier.srcAccessMask    = VK_ACCESS_TRANSFER_READ_BIT;
                    imageMemoryBarrier.dstAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                }

                mDeviceWrapper->endAndSubmitSingleTimeCommand(cmdBuf, mGraphicsQueue, false);
            }
        }

        {
            mDeviceWrapper->beginCommandBuffer(cmdBuf);
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.image            = cubemap->getImageHandle();
            imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask    = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            mDeviceWrapper->endAndSubmitSingleTimeCommand(cmdBuf, mGraphicsQueue, false);
        }

        vkDestroyRenderPass(device(), renderpass, nullptr);
        vkDestroyFramebuffer(device(), offscreen.framebuffer, nullptr);
        vkFreeMemory(device(), offscreen.memory, nullptr);
        vkDestroyImageView(device(), offscreen.view, nullptr);
        vkDestroyImage(device(), offscreen.image, nullptr);
        vkDestroyDescriptorPool(device(), descriptorpool, nullptr);
        vkDestroyDescriptorSetLayout(device(), descriptorsetlayout, nullptr);
        vkDestroyPipeline(device(), pipeline, nullptr);
        vkDestroyPipelineLayout(device(), pipelinelayout, nullptr);

        switch (target)
        {
            case IRRADIANCE:
                textures.irradianceCube = std::move(cubemap);
                break;
            case PREFILTEREDENV:
                textures.prefilteredCube                    = std::move(cubemap);
                shaderValuesParams.prefilteredCubeMipLevels = static_cast<float>(numMips);
                break;
        };

        auto tEnd  = std::chrono::high_resolution_clock::now();
        auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        LOGCATI("Generating cube map with %d  mip levels took %d ms", numMips, tDiff);
    }
}

void Sample_10_PBR::generateBRDFLUT()
{
    auto tStart = std::chrono::high_resolution_clock::now();

    const VkFormat format = VK_FORMAT_R16G16_SFLOAT;
    const int32_t  dim    = 512;

    gain::Image::ImageBasicInfo imageInfo = {
        format: format,
        imageType: VK_IMAGE_TYPE_2D,
        mipLevels: 1,
        arrayLayers: 1,
        usage: VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        layout: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    imageInfo.extent.width  = dim;
    imageInfo.extent.height = dim;
    imageInfo.extent.depth  = 1;
    textures.lutBrdf        = gain::Image::createDeviceLocal(deviceWrapper(), mGraphicsQueue, imageInfo);

    // FB, Att, RP, Pipe, etc.
    VkAttachmentDescription attDesc{};
    // Color attachment
    attDesc.format                       = format;
    attDesc.samples                      = VK_SAMPLE_COUNT_1_BIT;
    attDesc.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc.stencilLoadOp                = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc.stencilStoreOp               = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc.initialLayout                = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc.finalLayout                  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments    = &colorReference;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create the actual renderpass
    VkRenderPassCreateInfo renderPassCI{};
    renderPassCI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments    = &attDesc;
    renderPassCI.subpassCount    = 1;
    renderPassCI.pSubpasses      = &subpassDescription;
    renderPassCI.dependencyCount = 2;
    renderPassCI.pDependencies   = dependencies.data();

    VkRenderPass renderpass;
    CALL_VK(vkCreateRenderPass(device(), &renderPassCI, nullptr, &renderpass));

    VkFramebufferCreateInfo framebufferCI{};
    framebufferCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCI.renderPass      = renderpass;
    framebufferCI.attachmentCount = 1;
    auto imgView                  = textures.lutBrdf->getImageViewHandle();
    framebufferCI.pAttachments    = &imgView;
    framebufferCI.width           = dim;
    framebufferCI.height          = dim;
    framebufferCI.layers          = 1;

    VkFramebuffer framebuffer;
    CALL_VK(vkCreateFramebuffer(device(), &framebufferCI, nullptr, &framebuffer));

    // Desriptors
    VkDescriptorSetLayout           descriptorsetlayout;
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
    descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    CALL_VK(vkCreateDescriptorSetLayout(device(), &descriptorSetLayoutCI, nullptr, &descriptorsetlayout));

    // Pipeline layout
    VkPipelineLayout           pipelinelayout;
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = 1;
    pipelineLayoutCI.pSetLayouts    = &descriptorsetlayout;
    CALL_VK(vkCreatePipelineLayout(device(), &pipelineLayoutCI, nullptr, &pipelinelayout));

    // Pipeline
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
    inputAssemblyStateCI.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
    rasterizationStateCI.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCI.cullMode    = VK_CULL_MODE_NONE;
    rasterizationStateCI.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCI.lineWidth   = 1.0f;

    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachmentState.blendEnable    = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
    colorBlendStateCI.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCI.attachmentCount = 1;
    colorBlendStateCI.pAttachments    = &blendAttachmentState;

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
    depthStencilStateCI.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCI.depthTestEnable  = VK_FALSE;
    depthStencilStateCI.depthWriteEnable = VK_FALSE;
    depthStencilStateCI.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateCI.front            = depthStencilStateCI.back;
    depthStencilStateCI.back.compareOp   = VK_COMPARE_OP_ALWAYS;

    VkPipelineViewportStateCreateInfo viewportStateCI{};
    viewportStateCI.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCI.viewportCount = 1;
    viewportStateCI.scissorCount  = 1;

    VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
    multisampleStateCI.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkDynamicState>      dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateCI{};
    dynamicStateCI.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCI.pDynamicStates    = dynamicStateEnables.data();
    dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

    VkPipelineVertexInputStateCreateInfo emptyInputStateCI{};
    emptyInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.layout              = pipelinelayout;
    pipelineCI.renderPass          = renderpass;
    pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
    pipelineCI.pVertexInputState   = &emptyInputStateCI;
    pipelineCI.pRasterizationState = &rasterizationStateCI;
    pipelineCI.pColorBlendState    = &colorBlendStateCI;
    pipelineCI.pMultisampleState   = &multisampleStateCI;
    pipelineCI.pViewportState      = &viewportStateCI;
    pipelineCI.pDepthStencilState  = &depthStencilStateCI;
    pipelineCI.pDynamicState       = &dynamicStateCI;
    pipelineCI.stageCount          = 2;
    pipelineCI.pStages             = shaderStages.data();

    // Look-up-table (from BRDF) pipeline
    shaderStages = {
        loadShader("shaders/base/genbrdflut.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
        loadShader("shaders/base/genbrdflut.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};
    VkPipeline pipeline;
    CALL_VK(vkCreateGraphicsPipelines(device(), mPipelineCache.handle(), 1, &pipelineCI, nullptr, &pipeline));
    vks::debug::setPipelineName(device(), pipeline, "generateBRDFLUT_pipeline");
    for (auto shaderStage : shaderStages)
    {
        vkDestroyShaderModule(device(), shaderStage.module, nullptr);
    }

    // Render
    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass               = renderpass;
    renderPassBeginInfo.renderArea.extent.width  = dim;
    renderPassBeginInfo.renderArea.extent.height = dim;
    renderPassBeginInfo.clearValueCount          = 1;
    renderPassBeginInfo.pClearValues             = clearValues;
    renderPassBeginInfo.framebuffer              = framebuffer;

    VkCommandBuffer cmdBuf = mDeviceWrapper->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.width    = (float) dim;
    viewport.height   = (float) dim;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent.width  = dim;
    scissor.extent.height = dim;

    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDraw(cmdBuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdBuf);
    mDeviceWrapper->endAndSubmitSingleTimeCommand(cmdBuf, mGraphicsQueue);

    vkQueueWaitIdle(mGraphicsQueue);

    vkDestroyPipeline(device(), pipeline, nullptr);
    vkDestroyPipelineLayout(device(), pipelinelayout, nullptr);
    vkDestroyRenderPass(device(), renderpass, nullptr);
    vkDestroyFramebuffer(device(), framebuffer, nullptr);
    vkDestroyDescriptorSetLayout(device(), descriptorsetlayout, nullptr);

    auto tEnd  = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    LOGCATI("Generating BRDF LUT took %d ms", tDiff);
}

void Sample_10_PBR::preparePipelines()
{
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // The layout used for this pipeline (can be shared among multiple pipelines using the same
    // layout)
    pipelineCreateInfo.layout = mPipelineLayout.handle();
    // Renderpass this pipeline is attached to
    pipelineCreateInfo.renderPass = mRenderPass;

    // Construct the different states making up the pipeline

    // Input assembly state describes how primitives are assembled
    // This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode                            = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode                               = VK_CULL_MODE_NONE;
    rasterizationState.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthClampEnable                       = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable                = VK_FALSE;
    rasterizationState.depthBiasEnable                        = VK_FALSE;
    rasterizationState.lineWidth                              = 1.0f;

    // Color blend state describes how blend factors are calculated (if used)
    // We need one blend attachment state per color attachment (even if blending is not used)
    VkPipelineColorBlendAttachmentState blendAttachmentState = {};
    blendAttachmentState.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachmentState.blendEnable                         = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount                     = 1;
    colorBlendState.pAttachments                        = &blendAttachmentState;

    // Viewport state sets the number of viewports and scissor used in this pipeline
    // Note: This is actually overridden by the dynamic states (see below)
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount                     = 1;
    viewportState.scissorCount                      = 1;

    // Enable dynamic states
    // Most states are baked into the pipeline, but there are still a few dynamic states that can be
    // changed within a command buffer To be able to change these we need do specify which dynamic
    // states will be changed using this pipeline. Their actual states are set later on in the
    // command buffer. For this example we will set the viewport and scissor using dynamic states
    std::vector<VkDynamicState> dynamicStateEnables;
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates                   = dynamicStateEnables.data();
    dynamicState.dynamicStateCount                = static_cast<uint32_t>(dynamicStateEnables.size());

    // Depth and stencil state containing depth and stencil compare and test operations
    // We only use depth tests and want depth tests and writes to be enabled and compare with less
    // or equal
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable                       = VK_TRUE;
    depthStencilState.depthWriteEnable                      = VK_TRUE;
    depthStencilState.depthCompareOp                        = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable                 = VK_FALSE;
    depthStencilState.back.failOp                           = VK_STENCIL_OP_KEEP;
    depthStencilState.back.passOp                           = VK_STENCIL_OP_KEEP;
    depthStencilState.back.compareOp                        = VK_COMPARE_OP_ALWAYS;
    depthStencilState.stencilTestEnable                     = VK_FALSE;
    depthStencilState.front                                 = depthStencilState.back;

    // Multi sampling state
    // This example does not make use of multi sampling (for anti-aliasing), the state must still be
    // set and passed to the pipeline
    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.pSampleMask                          = nullptr;

    // Vertex input descriptions
    // Specifies the vertex input parameters for a pipeline

    // Vertex input bindings and attributes
    VkVertexInputBindingDescription vertexInputBinding = {0, sizeof(vkglTF::Model::Vertex), VK_VERTEX_INPUT_RATE_VERTEX};

    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vkglTF::Model::Vertex, uv0)},
        {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vkglTF::Model::Vertex, uv1)},
        {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vkglTF::Model::Vertex, joint0)},
        {5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vkglTF::Model::Vertex, weight0)}};

    // Vertex input state used for pipeline creation
    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount        = 1;
    vertexInputState.pVertexBindingDescriptions           = &vertexInputBinding;
    vertexInputState.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

    // Shaders
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

    // Vertex shader
    shaderStages[0] = loadShader("shaders/base/skybox.vert.spv",
                                 VK_SHADER_STAGE_VERTEX_BIT);
    // Fragment shader
    shaderStages[1] = loadShader("shaders/base/skybox.frag.spv",
                                 VK_SHADER_STAGE_FRAGMENT_BIT);

    // Set pipeline shader stage info
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages    = shaderStages.data();

    // Assign the pipeline states to the pipeline creation info structure
    pipelineCreateInfo.pVertexInputState   = &vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState    = &colorBlendState;
    pipelineCreateInfo.pMultisampleState   = &multisampleState;
    pipelineCreateInfo.pViewportState      = &viewportState;
    pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
    pipelineCreateInfo.renderPass          = mRenderPass;
    pipelineCreateInfo.pDynamicState       = &dynamicState;

    // Skybox pipeline (background cube)
    pipelines.skybox = VulkanPipeline(device());
    CALL_VK(vkCreateGraphicsPipelines(
        device(), mPipelineCache.handle(), 1, &pipelineCreateInfo, nullptr, pipelines.skybox.pHandle()));
    vks::debug::setPipelineName(device(), pipelines.skybox.handle(), "pipelines.skybox");

    // Shader modules are no longer needed once the graphics pipeline has been created
    vkDestroyShaderModule(device(), shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device(), shaderStages[1].module, nullptr);

    // PBR pipeline
    shaderStages[0]                    = loadShader(vertFilePath,
                                 VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1]                    = loadShader(fragFilePath,
                                 VK_SHADER_STAGE_FRAGMENT_BIT);
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthTestEnable  = VK_TRUE;
    pipelines.pbr                      = VulkanPipeline(device());
    CALL_VK(vkCreateGraphicsPipelines(device(), mPipelineCache.handle(), 1, &pipelineCreateInfo, nullptr, pipelines.pbr.pHandle()));
    vks::debug::setPipelineName(device(), pipelines.pbr.handle(), "pipelines.pbr");

    rasterizationState.cullMode              = VK_CULL_MODE_NONE;
    blendAttachmentState.blendEnable         = VK_TRUE;
    blendAttachmentState.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
    blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;
    pipelines.pbrAlphaBlend                  = VulkanPipeline(device());
    CALL_VK(vkCreateGraphicsPipelines(device(), mPipelineCache.handle(), 1, &pipelineCreateInfo, nullptr, pipelines.pbrAlphaBlend.pHandle()));
    vks::debug::setPipelineName(device(), pipelines.pbrAlphaBlend.handle(), "pipelines.pbrAlphaBlend");
}

void Sample_10_PBR::setupDescriptorSet()
{
    descriptorSets.resize(mSwapChain.imageCount);

    // Scene (matrices and environment maps)
    {
        for (auto i = 0; i < descriptorSets.size(); i++)
        {
            VkDescriptorSetAllocateInfo descriptorSetAllocInfo = vks::initializers::descriptorSetAllocateInfo(
                mDescriptorPool.handle(), descriptorSetLayouts.scene.pHandle(), 1);
            CALL_VK(vkAllocateDescriptorSets(device(), &descriptorSetAllocInfo, &descriptorSets[i].scene));
            vks::debug::setDescriptorSetName(device(), descriptorSets[i].scene, "descriptorSets[i].scene");

            auto                              sceneDesc           = uniformBuffers[i].scene->getDescriptor();
            auto                              paramsDesc          = uniformBuffers[i].params->getDescriptor();
            auto                              irradianceCubeDesc  = textures.irradianceCube->getDescriptor();
            auto                              prefilteredCubeDesc = textures.prefilteredCube->getDescriptor();
            auto                              lutBrdfDesc         = textures.lutBrdf->getDescriptor();
            std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                vks::initializers::writeDescriptorSet(descriptorSets[i].scene, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &sceneDesc),
                vks::initializers::writeDescriptorSet(descriptorSets[i].scene, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &paramsDesc),
                vks::initializers::writeDescriptorSet(descriptorSets[i].scene, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &irradianceCubeDesc),
                vks::initializers::writeDescriptorSet(descriptorSets[i].scene, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &prefilteredCubeDesc),
                vks::initializers::writeDescriptorSet(descriptorSets[i].scene, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &lutBrdfDesc),
            };

            vkUpdateDescriptorSets(device(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
        }
    }

    // Descriptor sets for materials
    {
        for (auto &material : pbrModels.scene.materials)
        {
            const VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(
                mDescriptorPool.handle(), descriptorSetLayouts.material.pHandle(), 1);
            CALL_VK(vkAllocateDescriptorSets(device(), &allocInfo, &material.descriptorSet));
            vks::debug::setDescriptorSetName(device(), material.descriptorSet, "material.descriptorSet");

            std::vector<VkDescriptorImageInfo> imageDescriptors(5);
            if (material.pbrWorkflows.metallicRoughness)
            {
                if (material.baseColorTexture)
                {
                    imageDescriptors[0] = material.baseColorTexture->descriptor;
                }
                if (material.metallicRoughnessTexture)
                {
                    imageDescriptors[1] = material.metallicRoughnessTexture->descriptor;
                }
            }

            if (material.pbrWorkflows.specularGlossiness)
            {
                if (material.extension.diffuseTexture)
                {
                    imageDescriptors[0] = material.extension.diffuseTexture->descriptor;
                }
                if (material.extension.specularGlossinessTexture)
                {
                    imageDescriptors[1] = material.extension.specularGlossinessTexture->descriptor;
                }
            }

            if (material.normalTexture)
            {
                imageDescriptors[2] = material.normalTexture->descriptor;
            }

            if (material.occlusionTexture)
            {
                imageDescriptors[3] = material.occlusionTexture->descriptor;
            }

            if (material.emissiveTexture)
            {
                imageDescriptors[4] = material.emissiveTexture->descriptor;
            }

            // TODO: Maybe need a default material?

            std::array<VkWriteDescriptorSet, 5> writeDescriptorSets{};
            for (size_t i = 0; i < imageDescriptors.size(); i++)
            {
                writeDescriptorSets[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSets[i].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeDescriptorSets[i].descriptorCount = 1;
                writeDescriptorSets[i].dstSet          = material.descriptorSet;
                writeDescriptorSets[i].dstBinding      = static_cast<uint32_t>(i);
                writeDescriptorSets[i].pImageInfo      = &imageDescriptors[i];
            }

            vkUpdateDescriptorSets(device(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
        }
    }

    // Descriptor sets for model node (matrices)
    {
        // Per-Node descriptor set
        for (auto &node : pbrModels.scene.nodes)
        {
            setupNodeDescriptorSet(node);
        }
    }

    // Skybox (fixed set)
    for (auto i = 0; i < uniformBuffers.size(); i++)
    {
        VkDescriptorSetAllocateInfo descriptorSetAllocInfo = vks::initializers::descriptorSetAllocateInfo(
            mDescriptorPool.handle(), descriptorSetLayouts.scene.pHandle(), 1);
        CALL_VK(vkAllocateDescriptorSets(device(), &descriptorSetAllocInfo, &descriptorSets[i].skybox));
        vks::debug::setDescriptorSetName(device(), descriptorSets[i].skybox, "descriptorSets[i].skybox");

        auto                              skyboxDesc          = uniformBuffers[i].skybox->getDescriptor();
        auto                              paramsDesc          = uniformBuffers[i].params->getDescriptor();
        auto                              prefilteredCubeDesc = textures.prefilteredCube->getDescriptor();
        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
            vks::initializers::writeDescriptorSet(descriptorSets[i].skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &skyboxDesc),
            vks::initializers::writeDescriptorSet(descriptorSets[i].skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &paramsDesc),
            vks::initializers::writeDescriptorSet(descriptorSets[i].skybox, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &prefilteredCubeDesc),
        };

        vkUpdateDescriptorSets(device(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }
}

void Sample_10_PBR::setupNodeDescriptorSet(vkglTF::Node *node)
{
    if (node->mesh)
    {
        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool     = mDescriptorPool.handle();
        descriptorSetAllocInfo.pSetLayouts        = descriptorSetLayouts.node.pHandle();
        descriptorSetAllocInfo.descriptorSetCount = 1;
        CALL_VK(vkAllocateDescriptorSets(device(), &descriptorSetAllocInfo, &node->mesh->uniformBuffer.descriptorSet));

        auto                 descriptor = node->mesh->uniformBuffer.buffer->getDescriptor();
        VkWriteDescriptorSet writeDescriptorSet =
            vks::initializers::writeDescriptorSet(node->mesh->uniformBuffer.descriptorSet,
                                                  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  0,
                                                  &descriptor);

        vkUpdateDescriptorSets(device(), 1, &writeDescriptorSet, 0, nullptr);
    }
    for (auto &child : node->children)
    {
        setupNodeDescriptorSet(child);
    }
}

void Sample_10_PBR::renderNode(vkglTF::Node *node, uint32_t cbIndex, vkglTF::Material::AlphaMode alphaMode)
{
    if (node->mesh)
    {
        // Render mesh primitives
        for (vkglTF::Primitive *primitive : node->mesh->primitives)
        {
            if (primitive->material.alphaMode == alphaMode)
            {
                const std::vector<VkDescriptorSet> descriptorsets = {
                    descriptorSets[cbIndex].scene,
                    primitive->material.descriptorSet,
                    node->mesh->uniformBuffer.descriptorSet,
                };

                vkCmdBindDescriptorSets(drawCmdBuffers[cbIndex].handle(),
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        mPipelineLayout.handle(),
                                        0,
                                        static_cast<uint32_t>(descriptorsets.size()),
                                        descriptorsets.data(),
                                        0,
                                        nullptr);

                // Pass material parameters as push constants
                PushConstBlockMaterial pushConstBlockMaterial{};
                pushConstBlockMaterial.emissiveFactor = primitive->material.emissiveFactor;
                // To save push constant space, availabilty and texture coordiante set are combined
                // -1 = texture not used for this material, >= 0 texture used and index of texture coordinate set
                pushConstBlockMaterial.colorTextureSet =
                    primitive->material.baseColorTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
                pushConstBlockMaterial.normalTextureSet =
                    primitive->material.normalTexture != nullptr ? primitive->material.texCoordSets.normal : -1;
                pushConstBlockMaterial.occlusionTextureSet =
                    primitive->material.occlusionTexture != nullptr ? primitive->material.texCoordSets.occlusion : -1;
                pushConstBlockMaterial.emissiveTextureSet =
                    primitive->material.emissiveTexture != nullptr ? primitive->material.texCoordSets.emissive : -1;
                pushConstBlockMaterial.alphaMask = static_cast<float>(
                    primitive->material.alphaMode == vkglTF::Material::ALPHAMODE_MASK);
                pushConstBlockMaterial.alphaMaskCutoff = primitive->material.alphaCutoff;

                // TODO: glTF specs states that metallic roughness should be preferred, even if specular glosiness is present

                if (primitive->material.pbrWorkflows.metallicRoughness)
                {
                    // Metallic roughness workflow
                    pushConstBlockMaterial.workflow        = static_cast<float>(PBR_WORKFLOW_METALLIC_ROUGHNESS);
                    pushConstBlockMaterial.baseColorFactor = primitive->material.baseColorFactor;
                    pushConstBlockMaterial.metallicFactor  = primitive->material.metallicFactor;
                    pushConstBlockMaterial.roughnessFactor = primitive->material.roughnessFactor;
                    pushConstBlockMaterial.PhysicalDescriptorTextureSet =
                        primitive->material.metallicRoughnessTexture != nullptr ? primitive->material.texCoordSets.metallicRoughness : -1;
                    pushConstBlockMaterial.colorTextureSet =
                        primitive->material.baseColorTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
                }

                if (primitive->material.pbrWorkflows.specularGlossiness)
                {
                    // Specular glossiness workflow
                    pushConstBlockMaterial.workflow = static_cast<float>(PBR_WORKFLOW_SPECULAR_GLOSINESS);
                    pushConstBlockMaterial.PhysicalDescriptorTextureSet =
                        primitive->material.extension.specularGlossinessTexture != nullptr ? primitive->material.texCoordSets.specularGlossiness : -1;
                    pushConstBlockMaterial.colorTextureSet =
                        primitive->material.extension.diffuseTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
                    pushConstBlockMaterial.diffuseFactor  = primitive->material.extension.diffuseFactor;
                    pushConstBlockMaterial.specularFactor = glm::vec4(
                        primitive->material.extension.specularFactor, 1.0f);
                }

                vkCmdPushConstants(drawCmdBuffers[cbIndex].handle(), mPipelineLayout.handle(), VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                   sizeof(PushConstBlockMaterial), &pushConstBlockMaterial);

                if (primitive->hasIndices)
                {
                    vkCmdDrawIndexed(drawCmdBuffers[cbIndex].handle(), primitive->indexCount, 1, primitive->firstIndex, 0, 0);
                }
                else
                {
                    vkCmdDraw(drawCmdBuffers[cbIndex].handle(), primitive->vertexCount, 1, 0, 0);
                }
            }
        }
    };
    for (auto child : node->children)
    {
        renderNode(child, cbIndex, alphaMode);
    }
}

void Sample_10_PBR::buildCommandBuffers()
{
    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext                    = nullptr;

    // Set clear values for all framebuffer attachments with loadOp set to clear
    // We use two attachments (color and depth) that are cleared at the start of the subpass and as
    // such we need to set clear values for both
    VkClearValue clearValues[2];
    clearValues[0].color        = {{0.8f, 0.8f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo    = {};
    renderPassBeginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext                    = nullptr;
    renderPassBeginInfo.renderPass               = mRenderPass;
    renderPassBeginInfo.renderArea.offset.x      = 0;
    renderPassBeginInfo.renderArea.offset.y      = 0;
    renderPassBeginInfo.renderArea.extent.width  = mWindow.windowWidth;
    renderPassBeginInfo.renderArea.extent.height = mWindow.windowHeight;
    renderPassBeginInfo.clearValueCount          = 2;
    renderPassBeginInfo.pClearValues             = clearValues;

    for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
    {
        if (vks::debug::debugable)
        {
            vks::debug::setCommandBufferName(device(), drawCmdBuffers[i].handle(), "drawCmdBuffers");
        }

        // Set target frame buffer
        renderPassBeginInfo.framebuffer = frameBuffers[i];

        CALL_VK(vkBeginCommandBuffer(drawCmdBuffers[i].handle(), &cmdBufInfo));

        // Start the first sub pass specified in our default prepare pass setup by the base class
        // This will clear the color and depth attachment
        vkCmdBeginRenderPass(
            drawCmdBuffers[i].handle(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Update dynamic viewport state
        VkViewport viewport = {};
        viewport.height     = (float) mWindow.windowHeight;
        viewport.width      = (float) mWindow.windowWidth;
        viewport.minDepth   = (float) 0.0f;
        viewport.maxDepth   = (float) 1.0f;
        vkCmdSetViewport(drawCmdBuffers[i].handle(), 0, 1, &viewport);

        // Update dynamic scissor state
        VkRect2D scissor      = {};
        scissor.extent.width  = mWindow.windowWidth;
        scissor.extent.height = mWindow.windowHeight;
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;
        vkCmdSetScissor(drawCmdBuffers[i].handle(), 0, 1, &scissor);

        VkDeviceSize offsets[1] = {0};

        if (displayBackground)
        {
            vkCmdBindDescriptorSets(drawCmdBuffers[i].handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout.handle(), 0, 1, &descriptorSets[i].skybox, 0, nullptr);
            vkCmdBindPipeline(drawCmdBuffers[i].handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skybox.handle());
            //            pbrModels.skybox.draw(drawCmdBuffers[i].handle());
        }

        vkCmdBindPipeline(
            drawCmdBuffers[i].handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.pbr.handle());

        vkglTF::Model &model = pbrModels.scene;

        auto vertexBuf = model.vertices.buffer->getBufferHandle();
        vkCmdBindVertexBuffers(drawCmdBuffers[i].handle(), 0, 1, &vertexBuf, offsets);
        if (model.indices.buffer != VK_NULL_HANDLE)
        {
            vkCmdBindIndexBuffer(drawCmdBuffers[i].handle(), model.indices.buffer->getBufferHandle(), 0, VK_INDEX_TYPE_UINT32);
        }

        // Opaque primitives first
        for (auto node : model.nodes)
        {
            renderNode(node, i, vkglTF::Material::ALPHAMODE_OPAQUE);
        }
        // Alpha masked primitives
        for (auto node : model.nodes)
        {
            renderNode(node, i, vkglTF::Material::ALPHAMODE_MASK);
        }
        // Transparent primitives
        // TODO: Correct depth sorting
        vkCmdBindPipeline(drawCmdBuffers[i].handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.pbrAlphaBlend.handle());
        for (auto node : model.nodes)
        {
            renderNode(node, i, vkglTF::Material::ALPHAMODE_BLEND);
        }

        //        drawUI(drawCmdBuffers[i].handle());

        vkCmdEndRenderPass(drawCmdBuffers[i].handle());

        // Ending the prepare pass will add an implicit barrier transitioning the frame buffer color
        // attachment to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

        CALL_VK(vkEndCommandBuffer(drawCmdBuffers[i].handle()));
    }
}

void Sample_10_PBR::prepareUniformBuffers()
{
    uniformBuffers.resize(mSwapChain.imageCount);
    for (auto &uniformBuffer : uniformBuffers)
    {
        uniformBuffer.scene = Buffer::create(deviceWrapper(),
                                             sizeof(shaderValuesScene),
                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        uniformBuffer.skybox = Buffer::create(deviceWrapper(),
                                              sizeof(shaderValuesSkybox),
                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        uniformBuffer.params = Buffer::create(deviceWrapper(),
                                              sizeof(shaderValuesParams),
                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    updateUniformBuffers();
}

void Sample_10_PBR::draw()
{
    VulkanContextBase::draw();
}

void Sample_10_PBR::onTouchActionMove(float deltaX, float deltaY)
{
    mCamera.rotate(glm::vec3(0.0f, -deltaX * mCamera.rotationSpeed * 0.1f, 0.0f));
}

void Sample_10_PBR::unInit(JNIEnv *env)
{}

Sample_10_PBR::~Sample_10_PBR()
{
    pbrModels.scene.destroy(device());
    pbrModels.skybox.destroy(device());
}