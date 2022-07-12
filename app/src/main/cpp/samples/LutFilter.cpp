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

#include "LutFilter.h"
#include "VulkanContextBase.h"
#include "includes/cube_data.h"
#include <VulkanInitializers.hpp>

LutFilter::LutFilter(const LutFilter &filter) :
    context(filter.context), descriptorPool(context->device()), descriptorSetLayout(context->device()), pipelineLayout(context->device()), pipeline(context->device())
{}

LutFilter::LutFilter(VulkanContextBase *c) :
    context(c), descriptorPool(context->device()), descriptorSetLayout(context->device()), pipelineLayout(context->device()), pipeline(context->device())
{}

void LutFilter::prepare(std::vector<VkDescriptorImageInfo> yuvDescriptors,
                        VkDescriptorImageInfo lutDescriptor, VkDescriptorBufferInfo bufDescriptor,
                        const VkPipelineCache pipelineCache, const VkRenderPass renderPass)
{
    descriptorPool      = VulkanDescriptorPool(context->device());
    descriptorSetLayout = VulkanDescriptorSetLayout(context->device());
    pipelineLayout      = VulkanPipelineLayout(context->device());
    pipeline            = VulkanPipeline(context->device());

    prepareVertices(true, g_vb_bitmap_texture_Data, sizeof(g_vb_bitmap_texture_Data));

    prepareResource(yuvDescriptors, lutDescriptor, bufDescriptor);

    shaders = {
        context->loadShader("shaders/shader_06_multi_lut.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
        context->loadShader("shaders/shader_06_multi_lut.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    preparePipeline(pipelineCache, renderPass);

    // Shader modules are no longer needed once the graphics pipeline has been created
    vkDestroyShaderModule(context->device(), shaders[0].module, nullptr);
    vkDestroyShaderModule(context->device(), shaders[1].module, nullptr);
}

void LutFilter::prepareVertices(bool useStagingBuffers, const void *data, size_t bufSize)
{
    // A note on memory management in Vulkan in general:
    //	This is a very complex topic and while it's fine for an example application to small
    // individual memory allocations that is not 	what should be done a real-world application, where
    // you should allocate large chunks of memory at once instead.

    // Setup vertices

    uint32_t vertexBufferSize = bufSize;

    if (useStagingBuffers)
    {
        // Static data like vertex and index buffer should be stored on the device memory
        // for optimal (and fastest) access by the GPU
        //
        // To achieve this we use so-called "staging buffers" :
        // - Create a buffer that's visible to the host (and can be mapped)
        // - Copy the data to this buffer
        // - Create another buffer that's local on the device (VRAM) with the same size
        // - Copy the data from the host to the device using a command buffer
        // - Delete the host visible (staging) buffer
        // - Use the device local buffers for rendering

        auto stagingBuffers = vks::Buffer::create(
            context->deviceWrapper(),
            vertexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        stagingBuffers->map();
        stagingBuffers->copyFrom(data, vertexBufferSize);
        stagingBuffers->unmap();

        mVerticesBuffer =
            vks::Buffer::create(context->deviceWrapper(),
                           vertexBufferSize,
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // Buffer copies have to be submitted to a queue, so we need a command buffer for them
        // Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that
        // may be faster when doing lots of copies
        VulkanCommandBuffer copyCmd(context->device(), context->commandPool());
        assert(context->deviceWrapper()->beginSingleTimeCommand(copyCmd.pHandle()));

        // Put buffer region copies into command buffer
        VkBufferCopy copyRegion = {};

        // Vertex buffer
        copyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(copyCmd.handle(),
                        stagingBuffers->getBufferHandle(),
                        mVerticesBuffer->getBufferHandle(),
                        1,
                        &copyRegion);

        // Flushing the command buffer will also submit it to the queue and uses a fence to ensure
        // that all commands have been executed before returning
        context->deviceWrapper()->endAndSubmitSingleTimeCommand(
            copyCmd.handle(), context->queue(), false);
    }
    else
    {
        // Don't use staging
        // Create host-visible buffers only and use these for rendering. This is not advised and
        // will usually result in lower rendering performance

        mVerticesBuffer = vks::Buffer::create(
            context->deviceWrapper(),
            vertexBufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        mVerticesBuffer->map();
        mVerticesBuffer->copyFrom(data, vertexBufferSize);
        mVerticesBuffer->unmap();
    }
}

void LutFilter::prepareResource(std::vector<VkDescriptorImageInfo> yuvDescriptors,
                                VkDescriptorImageInfo              lutDescriptor,
                                VkDescriptorBufferInfo             bufDescriptor)
{
    // Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4)};
    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vks::initializers::descriptorPoolCreateInfo(poolSizes, 5);
    CALL_VK(vkCreateDescriptorPool(
        context->device(), &descriptorPoolInfo, nullptr, descriptorPool.pHandle()));

    // Descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 3),
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    CALL_VK(vkCreateDescriptorSetLayout(
        context->device(), &descriptorLayout, nullptr, descriptorSetLayout.pHandle()));

    // Descriptor set
    VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(
        descriptorPool.handle(), descriptorSetLayout.pHandle(), 1);
    CALL_VK(vkAllocateDescriptorSets(context->device(), &allocInfo, &descriptorSet));

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        vks::initializers::writeDescriptorSet(
            descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &bufDescriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, yuvDescriptors.data(), 3),
        vks::initializers::writeDescriptorSet(
            descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &lutDescriptor)};
    vkUpdateDescriptorSets(context->device(),
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(),
                           0,
                           nullptr);
}

void LutFilter::preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass)
{
    // Pipeline layout
    // Push constants for UI rendering parameters
    VkPushConstantRange pushConstantRange = vks::initializers::pushConstantRange(
        VK_SHADER_STAGE_VERTEX_BIT, sizeof(LutPushConstantData), 0);
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
        vks::initializers::pipelineLayoutCreateInfo(descriptorSetLayout.pHandle(), 1);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstantRange;
    CALL_VK(vkCreatePipelineLayout(
        context->device(), &pipelineLayoutCreateInfo, nullptr, pipelineLayout.pHandle()));

    // Setup graphics pipeline
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizationState =
        vks::initializers::pipelineRasterizationStateCreateInfo(
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    // Enable blending
    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.blendEnable    = VK_TRUE;
    blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
    blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendState =
        vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        vks::initializers::pipelineDepthStencilStateCreateInfo(
            VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

    VkPipelineViewportStateCreateInfo viewportState =
        vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

    VkPipelineMultisampleStateCreateInfo multisampleState =
        vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

    std::vector<VkDynamicState>      dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT,
                                                       VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

    std::vector<VkSpecializationMapEntry> specializationMapEntries;
    specializationMapEntries.push_back(vks::initializers::specializationMapEntry(
        0, offsetof(SpecializationData, LUT_ITEM_INDEX), sizeof(float_t)));

    VkSpecializationInfo specializationInfo = vks::initializers::specializationInfo(
        static_cast<uint32_t>(specializationMapEntries.size()),
        specializationMapEntries.data(),
        sizeof(specializationData),
        &specializationData);
    shaders[0].pSpecializationInfo = &specializationInfo;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo =
        vks::initializers::pipelineCreateInfo(pipelineLayout.handle(), renderPass);

    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState    = &colorBlendState;
    pipelineCreateInfo.pMultisampleState   = &multisampleState;
    pipelineCreateInfo.pViewportState      = &viewportState;
    pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
    pipelineCreateInfo.pDynamicState       = &dynamicState;
    pipelineCreateInfo.stageCount          = static_cast<uint32_t>(shaders.size());
    pipelineCreateInfo.pStages             = shaders.data();
    pipelineCreateInfo.subpass             = 0;

    // Vertex bindings an attributes based on ImGui vertex definition
    std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
        vks::initializers::vertexInputBindingDescription(
            0, sizeof(VertexUV), VK_VERTEX_INPUT_RATE_VERTEX),
    };
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
        vks::initializers::vertexInputAttributeDescription(
            0,
            0,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            offsetof(VertexUV, posX)),        // Location 0: Position
        vks::initializers::vertexInputAttributeDescription(
            0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexUV, u)),        // Location 1: UV
    };
    VkPipelineVertexInputStateCreateInfo vertexInputState =
        vks::initializers::pipelineVertexInputStateCreateInfo();
    vertexInputState.vertexBindingDescriptionCount =
        static_cast<uint32_t>(vertexInputBindings.size());
    vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
    vertexInputState.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

    pipelineCreateInfo.pVertexInputState = &vertexInputState;

    CALL_VK(vkCreateGraphicsPipelines(
        context->device(), pipelineCache, 1, &pipelineCreateInfo, nullptr, pipeline.pHandle()));
}

void LutFilter::draw(const VkCommandBuffer commandBuffer, float_t itemWidth, float_t windowWidth)
{
    // Bind descriptor sets describing shader binding points
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout.handle(),
                            0,
                            1,
                            &descriptorSet,
                            0,
                            nullptr);

    // Bind push constant
    LutPushConstantData pushConstantData = {itemWidth, windowWidth};
    vkCmdPushConstants(commandBuffer,
                       pipelineLayout.handle(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0,
                       sizeof(LutPushConstantData),
                       &pushConstantData);

    // Bind the rendering pipeline
    // The pipeline (state object) contains all states of the rendering pipeline, binding it will
    // set all the states specified at pipeline creation time
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle());

    // Bind triangle vertex buffer (contains position and colors)
    VkDeviceSize offsets[1]  = {0};
    auto         verticesBuf = mVerticesBuffer->getBufferHandle();
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &verticesBuf, offsets);

    // Draw triangle
    vkCmdDraw(commandBuffer,
              sizeof(g_vb_bitmap_texture_Data) / sizeof(g_vb_bitmap_texture_Data[0]),
              1,
              0,
              0);
}

LutFilter::~LutFilter()
{}
