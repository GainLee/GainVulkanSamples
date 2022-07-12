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

#include "Sample_07_Histogram.h"

#include "includes/cube_data.h"

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Sample_07_Histogram::setYUVImage(uint8_t *yData, uint8_t *uData, uint8_t *vData, uint32_t w,
                                      uint32_t h, uint32_t yStride, uint32_t uStride,
                                      uint32_t vStride, uint32_t uPixelStride,
                                      uint32_t vPixelStride, uint32_t orientation)
{
    mYPlane = {
        .data        = yData,
        .w           = w,
        .h           = h,
        .stride      = yStride,
        .orientation = orientation,
    };
    mUPlane = {
        .data        = uData,
        .w           = w,
        .h           = h / 2,
        .stride      = uStride,
        .pixelStride = uPixelStride,
        .orientation = orientation,
    };
    mVPlane = {
        .data        = vData,
        .w           = w,
        .h           = h / 2,
        .stride      = vStride,
        .pixelStride = vPixelStride,
        .orientation = orientation,
    };
}

void Sample_07_Histogram::prepareImages(JNIEnv *env)
{
    Image::ImageBasicInfo imageInfo = {
        extent: {mYPlane.w, mYPlane.h, 1},
        usage: VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        format: VK_FORMAT_R8_UNORM,
        layout: VK_IMAGE_LAYOUT_GENERAL
    };
    mYImage = Image::createDeviceLocal(deviceWrapper(),
                                       mGraphicsQueue,
                                       imageInfo);

    imageInfo.extent = {mUPlane.w, mUPlane.h, 1};
    mUImage          = Image::createDeviceLocal(deviceWrapper(),
                                       mGraphicsQueue,
                                       imageInfo);

    imageInfo.extent = {mVPlane.w, mVPlane.h, 1};
    mVImage          = Image::createDeviceLocal(deviceWrapper(),
                                       mGraphicsQueue,
                                       imageInfo);

    mStorageBuffer =
        vks::Buffer::create(deviceWrapper(),
                       sizeof(int) * 256,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Sample_07_Histogram::prepare(JNIEnv *env)
{
    if (!mPrepared)
    {
        VulkanContextBase::prepare(env);

        prepareImages(env);

        setupDescriptorPool();

        prepareSynchronizationPrimitives();

        prepareGraphics();

        prepareCompute();

        mPrepared = true;
    }

    updateTexture(env);
}

void Sample_07_Histogram::prepareGraphics()
{
    prepareVertices(true, g_vb_bitmap_texture_Data, sizeof(g_vb_bitmap_texture_Data));
    prepareUniformBuffers();
    setupDescriptorSetLayout();
    setupDescriptorSet();
    preparePipelines();

    buildCommandBuffers();
}

void Sample_07_Histogram::prepareCompute()
{
    compute.queueFamilyIndex = deviceWrapper()->queueFamilyIndices.compute;
    vkGetDeviceQueue(device(), compute.queueFamilyIndex, 0, &compute.queue);

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // Binding 0
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0),
        // Binding 1
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1),
        // Binding 2
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2),
        // Binding 3
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 3),
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vks::initializers::descriptorSetLayoutCreateInfo(
            setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));

    CALL_VK(vkCreateDescriptorSetLayout(
        device(), &descriptorLayout, nullptr, &compute.descriptorSetLayout));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
        vks::initializers::pipelineLayoutCreateInfo(&compute.descriptorSetLayout, 1);

    struct pixelStride
    {
        int uPixelStride;
        int vPixelStride;
    };
    const VkPushConstantRange pushConstantRange =
        vks::initializers::pushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(pixelStride), 0);

    pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pPipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstantRange;

    CALL_VK(vkCreatePipelineLayout(
        device(), &pPipelineLayoutCreateInfo, nullptr, &compute.pipelineLayout));

    VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(
        descriptorPool(), &compute.descriptorSetLayout, 1);

    CALL_VK(vkAllocateDescriptorSets(device(), &allocInfo, &compute.descriptorSet));

    auto                              yDes                       = mYImage->getDescriptor();
    auto                              uDes                       = mUImage->getDescriptor();
    auto                              vDes                       = mVImage->getDescriptor();
    auto                              vStorageDes                = mStorageBuffer->getDescriptor();
    std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets = {
        // Binding 0 : YImage
        vks::initializers::writeDescriptorSet(
            compute.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, &yDes),
        // Binding 1 : UImage
        vks::initializers::writeDescriptorSet(
            compute.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &uDes),
        // Binding 2 : VImage
        vks::initializers::writeDescriptorSet(
            compute.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2, &vDes),
        // Binding 3 : Storage Image
        vks::initializers::writeDescriptorSet(
            compute.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &vStorageDes)};

    vkUpdateDescriptorSets(device(),
                           static_cast<uint32_t>(computeWriteDescriptorSets.size()),
                           computeWriteDescriptorSets.data(),
                           0,
                           nullptr);

    // Create pipelines
    VkComputePipelineCreateInfo computePipelineCreateInfo =
        vks::initializers::computePipelineCreateInfo(compute.pipelineLayout, 0);

    const auto                                  workGroupSize        = deviceWrapper()->workGroupSize;
    const uint32_t                              specializationData[] = {workGroupSize, workGroupSize};
    const std::vector<VkSpecializationMapEntry> specializationMap    = {
        // clang-format off
            // constantID, offset,               size
            {0, 0 * sizeof(uint32_t), sizeof(uint32_t)},
            {1, 1 * sizeof(uint32_t), sizeof(uint32_t)},
        // clang-format on
    };
    const VkSpecializationInfo specializationInfo = {
        .mapEntryCount = static_cast<uint32_t>(specializationMap.size()),
        .pMapEntries   = specializationMap.data(),
        .dataSize      = sizeof(specializationData),
        .pData         = specializationData,
    };

    computePipelineCreateInfo.stage                     = loadShader(mComputeShaderPath, VK_SHADER_STAGE_COMPUTE_BIT);
    computePipelineCreateInfo.stage.pSpecializationInfo = &specializationInfo;

    CALL_VK(vkCreateComputePipelines(device(),
                                     mPipelineCache.handle(),
                                     1,
                                     &computePipelineCreateInfo,
                                     nullptr,
                                     &compute.pipelineYuvToRgba));

    // Separate command pool as queue family for compute may be different than graphics
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex        = compute.queueFamilyIndex;
    cmdPoolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    CALL_VK(vkCreateCommandPool(device(), &cmdPoolInfo, nullptr, &compute.commandPool));

    // Create a command buffer for compute operations
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(
        compute.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    CALL_VK(vkAllocateCommandBuffers(device(), &cmdBufAllocateInfo, &compute.commandBuffer));

    // Semaphore for compute & graphics sync
    VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
    CALL_VK(vkCreateSemaphore(device(), &semaphoreCreateInfo, nullptr, &compute.semaphore));

    // Signal the semaphore
    VkSubmitInfo submitInfo         = vks::initializers::submitInfo();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &compute.semaphore;
    CALL_VK(vkQueueSubmit(compute.queue, 1, &submitInfo, VK_NULL_HANDLE));
    CALL_VK(vkQueueWaitIdle(compute.queue));

    buildComputeCommandBuffer();
}

void Sample_07_Histogram::buildComputeCommandBuffer()
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    CALL_VK(vkBeginCommandBuffer(compute.commandBuffer, &cmdBufInfo));

    vkCmdBindPipeline(
        compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelineYuvToRgba);
    vkCmdBindDescriptorSets(compute.commandBuffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            compute.pipelineLayout,
                            0,
                            1,
                            &compute.descriptorSet,
                            0,
                            0);

    // Bind push constant
    struct UVPixelStride
    {
        int uPixelStride;
        int vPixelStride;
    } pixelStride;

    pixelStride.uPixelStride = mUPlane.pixelStride;
    pixelStride.vPixelStride = mVPlane.pixelStride;
    vkCmdPushConstants(compute.commandBuffer,
                       compute.pipelineLayout,
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0,
                       sizeof(UVPixelStride),
                       &pixelStride);

    // Buffer初始用0填充，否则后面帧的buffer会被前面帧的污染
    vkCmdFillBuffer(compute.commandBuffer, mStorageBuffer->getBufferHandle(), 0, VK_WHOLE_SIZE, 0);

    const auto     workGroupSize = deviceWrapper()->workGroupSize;
    const uint32_t groupCountX   = (mYImage->width() + workGroupSize - 1) / workGroupSize;
    const uint32_t groupCountY   = (mYImage->height() + workGroupSize - 1) / workGroupSize;
    vkCmdDispatch(compute.commandBuffer, groupCountX, groupCountY, 1);

    vkEndCommandBuffer(compute.commandBuffer);
}

void Sample_07_Histogram::updateTexture(JNIEnv *env)
{
    mYImage->setContentFromBytes(mYPlane.data, mYPlane.stride * mYPlane.h, mYPlane.stride);

    mUImage->setContentFromBytes(mUPlane.data, mUPlane.stride * mUPlane.h, mUPlane.stride);

    mVImage->setContentFromBytes(mVPlane.data, mVPlane.stride * mVPlane.h, mVPlane.stride);
}

void Sample_07_Histogram::setupDescriptorPool()
{
    // We need to tell the API the number of max. requested descriptors per type
    VkDescriptorPoolSize typeCounts[2];
    // 三个采样Image，即Y、U、V
    typeCounts[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    typeCounts[0].descriptorCount = 2;
    // 一个Storage Image用于存储RGBA
    typeCounts[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    typeCounts[1].descriptorCount = 3;

    // Create the global descriptor pool
    // All descriptors used in this example are allocated from this pool
    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pNext                      = nullptr;
    descriptorPoolInfo.poolSizeCount              = 2;
    descriptorPoolInfo.pPoolSizes                 = typeCounts;
    // Set the max. number of descriptor sets that can be requested from this pool (requesting
    // beyond this limit will result in an error)
    descriptorPoolInfo.maxSets = 4;

    CALL_VK(
        vkCreateDescriptorPool(device(), &descriptorPoolInfo, nullptr, mDescriptorPool.pHandle()));
}

void Sample_07_Histogram::setupDescriptorSetLayout()
{
    // Setup layout of descriptors used in this example
    // Basically connects the different shader stages to descriptors for binding uniform buffers,
    // image samplers, etc. So every shader binding should map to one descriptor set layout binding

    VkDescriptorSetLayoutBinding layoutBinding = vks::initializers::descriptorSetLayoutBinding(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

    VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
    descriptorLayout.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayout.bindingCount                    = 1;
    descriptorLayout.pBindings                       = &layoutBinding;

    CALL_VK(vkCreateDescriptorSetLayout(
        device(), &descriptorLayout, nullptr, mDescriptorSetLayout.pHandle()));

    // Create the pipeline layout that is used to generate the rendering pipelines that are based on
    // this descriptor set layout In a more complex scenario you would have different pipeline
    // layouts for different descriptor set layouts that could be reused
    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
        vks::initializers::pipelineLayoutCreateInfo(mDescriptorSetLayout.pHandle(), 1);

    CALL_VK(vkCreatePipelineLayout(
        device(), &pPipelineLayoutCreateInfo, nullptr, mPipelineLayout.pHandle()));
}

void Sample_07_Histogram::setupDescriptorSet()
{
    // Allocate a new descriptor set from the global descriptor pool
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool              = mDescriptorPool.handle();
    allocInfo.descriptorSetCount          = 1;
    allocInfo.pSetLayouts                 = mDescriptorSetLayout.pHandle();

    CALL_VK(vkAllocateDescriptorSets(device(), &allocInfo, &mDescriptorSet));

    // Update the descriptor set determining the shader binding points
    // For every binding point used in a shader there needs to be one
    // descriptor set matching that binding point

    auto                 desc               = mStorageBuffer->getDescriptor();
    VkWriteDescriptorSet writeDescriptorSet = vks::initializers::writeDescriptorSet(
        mDescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &desc);

    vkUpdateDescriptorSets(device(), 1, &writeDescriptorSet, 0, nullptr);
}

void Sample_07_Histogram::prepareSynchronizationPrimitives()
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

    mGraphicsSemaphore = VulkanSemaphore(device());
    CALL_VK(
        vkCreateSemaphore(device(), &semaphoreCreateInfo, nullptr, mGraphicsSemaphore.pHandle()));
}

void Sample_07_Histogram::prepareUniformBuffers()
{
    // Prepare and initialize a uniform buffer block containing shader uniforms
    // Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are
    // passed via uniform buffer blocks
    mUniformBuffer =
        vks::Buffer::create(deviceWrapper(),
                       sizeof(uboVS),
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    mUniformBuffer->map();

    updateUniformBuffers();
}

void Sample_07_Histogram::updateUniformBuffers()
{
    float winRatio =
        static_cast<float>(mWindow.windowWidth) / static_cast<float>(mWindow.windowHeight);

    uint32_t bmpWidth  = mYImage->width();
    uint32_t bmpHeight = mYImage->height();

    // Pass matrices to the shaders
    uboVS.projectionMatrix = glm::mat4(1.0f);
    uboVS.viewMatrix       = glm::mat4(1.0f);

    if (mYPlane.orientation % 180 != 0)
    {
        uint32_t temp = bmpWidth;
        bmpWidth      = bmpHeight;
        bmpHeight     = temp;
    }

    float bmpRatio = static_cast<float>(bmpWidth) / static_cast<float>(bmpHeight);

    if (bmpRatio >= winRatio)
    {
        // The bitmap's width is to large and the width is compressed, so we compress the height
        uboVS.modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, winRatio / bmpRatio, 1.0f));
    }
    else
    {
        // The bitmap's height is to large and the height is compressed, so we compress the width
        uboVS.modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(bmpRatio / winRatio, 1.0f, 1.0f));
    }

    uboVS.modelMatrix = glm::rotate(
        uboVS.modelMatrix, glm::radians((float) mYPlane.orientation), glm::vec3(0.0f, 0.0f, 1.0f));

    mUniformBuffer->copyFrom(&uboVS, sizeof(uboVS));
}

void Sample_07_Histogram::preparePipelines()
{
    // Create the graphics pipeline used in this example
    // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing
    // OpenGL's complex state machine A pipeline is then stored and hashed on the GPU making
    // pipeline changes very fast Note: There are still a few dynamic states that are not directly
    // part of the pipeline (but the info that they are used is)

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
    VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
    blendAttachmentState[0].colorWriteMask                      = 0xf;
    blendAttachmentState[0].blendEnable                         = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo colorBlendState         = {};
    colorBlendState.sType                                       = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount                             = 1;
    colorBlendState.pAttachments                                = blendAttachmentState;

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

    // Vertex input binding
    // This example uses a single vertex input binding at binding point 0 (see
    // vkCmdBindVertexBuffers)
    VkVertexInputBindingDescription vertexInputBinding = {};
    vertexInputBinding.binding                         = 0;
    vertexInputBinding.stride                          = sizeof(VertexUV);
    vertexInputBinding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

    // Input attribute bindings describe shader attribute locations and memory layouts
    std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs;
    // These match the following shader layout (see shader_01_triangle.vert):
    //	layout (location = 0) in vec4 inPos;
    //	layout (location = 1) in vec2 inUVPos;
    // Attribute location 0: Position
    vertexInputAttributs[0].binding  = 0;
    vertexInputAttributs[0].location = 0;
    // Position attribute is four 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexInputAttributs[0].offset = offsetof(VertexUV, posX);
    // Attribute location 1: Color
    vertexInputAttributs[1].binding  = 0;
    vertexInputAttributs[1].location = 1;
    // Color attribute is two 32 bit signed (SFLOAT) floats (R32 G32)
    vertexInputAttributs[1].format = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributs[1].offset = offsetof(VertexUV, u);

    // Vertex input state used for pipeline creation
    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount        = 1;
    vertexInputState.pVertexBindingDescriptions           = &vertexInputBinding;
    vertexInputState.vertexAttributeDescriptionCount      = 2;
    vertexInputState.pVertexAttributeDescriptions         = vertexInputAttributs.data();

    // Shaders
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

    // Vertex shader
    shaderStages[0] = loadShader(vertFilePath, VK_SHADER_STAGE_VERTEX_BIT);
    // Fragment shader
    shaderStages[1] = loadShader(fragFilePath, VK_SHADER_STAGE_FRAGMENT_BIT);

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

    // Create rendering pipeline using the specified states
    CALL_VK(vkCreateGraphicsPipelines(
        device(), mPipelineCache.handle(), 1, &pipelineCreateInfo, nullptr, mPipeline.pHandle()));

    // Shader modules are no longer needed once the graphics pipeline has been created
    vkDestroyShaderModule(device(), shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device(), shaderStages[1].module, nullptr);
}

void Sample_07_Histogram::buildCommandBuffers()
{
    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext                    = nullptr;

    // Set clear values for all framebuffer attachments with loadOp set to clear
    // We use two attachments (color and depth) that are cleared at the start of the subpass and as
    // such we need to set clear values for both
    VkClearValue clearValues[2];
    clearValues[0].color        = {{0.2f, 0.2f, 0.2f, 0.5f}};
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

        // Bind descriptor sets describing shader binding points
        vkCmdBindDescriptorSets(drawCmdBuffers[i].handle(),
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mPipelineLayout.handle(),
                                0,
                                1,
                                &mDescriptorSet,
                                0,
                                nullptr);

        // Bind the rendering pipeline
        // The pipeline (state object) contains all states of the rendering pipeline, binding it
        // will set all the states specified at pipeline creation time
        vkCmdBindPipeline(
            drawCmdBuffers[i].handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.handle());

        // Bind triangle vertex buffer (contains position and colors)
        VkDeviceSize offsets[1]  = {0};
        auto         verticesBuf = mVerticesBuffer->getBufferHandle();
        vkCmdBindVertexBuffers(drawCmdBuffers[i].handle(), 0, 1, &verticesBuf, offsets);

        // Draw triangle
        vkCmdDraw(drawCmdBuffers[i].handle(),
                  sizeof(g_vb_bitmap_texture_Data) / sizeof(g_vb_bitmap_texture_Data[0]),
                  1,
                  0,
                  0);

        //        drawUI(drawCmdBuffers[i].handle());

        vkCmdEndRenderPass(drawCmdBuffers[i].handle());

        // Ending the prepare pass will add an implicit barrier transitioning the frame buffer color
        // attachment to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

        CALL_VK(vkEndCommandBuffer(drawCmdBuffers[i].handle()));
    }
}

void Sample_07_Histogram::draw()
{
    prepareFrame();

    // Use a fence to wait until the command buffer has finished execution before using it again
    CALL_VK(vkWaitForFences(device(), 1, waitFences[currentBuffer].pHandle(), VK_TRUE, UINT64_MAX));
    CALL_VK(vkResetFences(device(), 1, waitFences[currentBuffer].pHandle()));

    // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
    VkPipelineStageFlags graphicsWaitStageMask[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSemaphore graphicsWaitSemaphores[] = {compute.semaphore, presentCompleteSemaphore.handle()};
    VkSemaphore graphicsSemaphores[]     = {mGraphicsSemaphore.handle(),
                                        renderCompleteSemaphore.handle()};

    // The submit info structure specifies a command buffer queue submission batch
    VkSubmitInfo submitInfo = {};
    submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask =
        graphicsWaitStageMask;        // Pointer to the list of pipeline stages that the semaphore waits
                                      // will occur at
    submitInfo.pWaitSemaphores =
        graphicsWaitSemaphores;               // Semaphore(s) to wait upon before the submitted command buffer
                                              // starts executing
    submitInfo.waitSemaphoreCount = 2;        // One wait semaphore
    submitInfo.pSignalSemaphores =
        graphicsSemaphores;                     // Semaphore(s) to be signaled when command buffers have completed
    submitInfo.signalSemaphoreCount = 2;        // One signal semaphore
    submitInfo.pCommandBuffers =
        drawCmdBuffers[currentBuffer]
            .pHandle();                       // Command buffers(s) to execute in this batch (submission)
    submitInfo.commandBufferCount = 1;        // One command buffer

    // Submit to the graphics queue passing a wait fence
    CALL_VK(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

    submitFrame();

    // Wait for rendering finished
    VkPipelineStageFlags computerWaitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    // Submit compute commands
    VkSubmitInfo computeSubmitInfo         = vks::initializers::submitInfo();
    computeSubmitInfo.commandBufferCount   = 1;
    computeSubmitInfo.pCommandBuffers      = &compute.commandBuffer;
    computeSubmitInfo.waitSemaphoreCount   = 1;
    computeSubmitInfo.pWaitSemaphores      = mGraphicsSemaphore.pHandle();        //&graphics.semaphore;
    computeSubmitInfo.pWaitDstStageMask    = &computerWaitStageMask;
    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.pSignalSemaphores    = &compute.semaphore;
    CALL_VK(
        vkQueueSubmit(compute.queue, 1, &computeSubmitInfo, waitFences[currentBuffer].handle()));
}

void Sample_07_Histogram::unInit(JNIEnv *env)
{
    for (auto item : mGlobalBitmaps)
    {
        env->DeleteGlobalRef(item);
    }
}

Sample_07_Histogram::~Sample_07_Histogram()
{}