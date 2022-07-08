/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 by Gain
 * Copyright (C) 2022 by Sascha Willems - www.saschawillems.de
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

#include "VulkanUIOverlay.h"
#include "VulkanContextBase.h"
#include "VulkanInitializers.hpp"

namespace vks
{
void UIOverlay::init()
{
    if (screenDensity >= ACONFIGURATION_DENSITY_XXHIGH)
    {
        scale = 3.5f;
    }
    else if (screenDensity >= ACONFIGURATION_DENSITY_XHIGH)
    {
        scale = 2.5f;
    }
    else if (screenDensity >= ACONFIGURATION_DENSITY_HIGH)
    {
        scale = 2.0f;
    };

    // Init ImGui
    ImGui::CreateContext();
    // Dimensions
    ImGuiIO &io = ImGui::GetIO();
    //		io.FontGlobalScale = scale;

    initRAIIObjects();
}

void UIOverlay::initRAIIObjects()
{
    descriptorPool      = VulkanDescriptorPool(deviceWrapper->logicalDevice);
    descriptorSetLayout = VulkanDescriptorSetLayout(deviceWrapper->logicalDevice);
    pipelineLayout      = VulkanPipelineLayout(deviceWrapper->logicalDevice);
    pipeline            = VulkanPipeline(deviceWrapper->logicalDevice);
}

/** Prepare all vulkan resources required to prepare the UI overlay */
void UIOverlay::prepareResources()
{
    ImGuiIO &io = ImGui::GetIO();

    // Create font texture
    unsigned char *fontData;
    int            texWidth, texHeight;

    float   scale = (float) screenDensity / (float) ACONFIGURATION_DENSITY_MEDIUM;
    AAsset *asset = AAssetManager_open(assetManager, "Roboto-Medium.ttf", AASSET_MODE_STREAMING);
    if (asset)
    {
        size_t size = AAsset_getLength(asset);
        assert(size > 0);
        char *fontAsset = new char[size];
        AAsset_read(asset, fontAsset, size);
        AAsset_close(asset);
        io.Fonts->AddFontFromMemoryTTF(fontAsset, size, 12.0f * scale);
        // fontAsset will be deleted by freeResources method
        // delete[] fontAsset;
    }

    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
    VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

    // Create target image for copy
    Image::ImageBasicInfo imageInfo = {
        extent: {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1},
        usage: VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    };
    fontImage =
        Image::createDeviceLocal(deviceWrapper,
                                 queue,
                                 imageInfo);
    fontImage->setContentFromBytes(fontData, uploadSize, texWidth);

    // Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};
    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
    CALL_VK(vkCreateDescriptorPool(
        deviceWrapper->logicalDevice, &descriptorPoolInfo, nullptr, descriptorPool.pHandle()));

    // Descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    CALL_VK(vkCreateDescriptorSetLayout(
        deviceWrapper->logicalDevice, &descriptorLayout, nullptr, descriptorSetLayout.pHandle()));

    // Descriptor set
    VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(
        descriptorPool.handle(), descriptorSetLayout.pHandle(), 1);
    CALL_VK(vkAllocateDescriptorSets(deviceWrapper->logicalDevice, &allocInfo, &descriptorSet));
    VkDescriptorImageInfo             fontDescriptor      = fontImage->getDescriptor();
    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {vks::initializers::writeDescriptorSet(
        descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor)};
    vkUpdateDescriptorSets(deviceWrapper->logicalDevice,
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(),
                           0,
                           nullptr);
}

/** Prepare a separate pipeline for the UI overlay rendering decoupled from the main application */
void UIOverlay::preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass)
{
    // Pipeline layout
    // Push constants for UI rendering parameters
    VkPushConstantRange pushConstantRange =
        vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
        vks::initializers::pipelineLayoutCreateInfo(descriptorSetLayout.pHandle(), 1);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstantRange;
    CALL_VK(vkCreatePipelineLayout(deviceWrapper->logicalDevice,
                                   &pipelineLayoutCreateInfo,
                                   nullptr,
                                   pipelineLayout.pHandle()));

    // Setup graphics pipeline for UI rendering
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
        vks::initializers::pipelineMultisampleStateCreateInfo(rasterizationSamples);

    std::vector<VkDynamicState>      dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT,
                                                       VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

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
    pipelineCreateInfo.subpass             = subpass;

    // Vertex bindings an attributes based on ImGui vertex definition
    std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
        vks::initializers::vertexInputBindingDescription(
            0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
    };
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
        vks::initializers::vertexInputAttributeDescription(
            0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),        // Location 0: Position
        vks::initializers::vertexInputAttributeDescription(
            0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),        // Location 1: UV
        vks::initializers::vertexInputAttributeDescription(
            0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),        // Location 0: Color
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

    CALL_VK(vkCreateGraphicsPipelines(deviceWrapper->logicalDevice,
                                      pipelineCache,
                                      1,
                                      &pipelineCreateInfo,
                                      nullptr,
                                      pipeline.pHandle()));
}

/** Update vertex and index buffer containing the imGui elements when required */
bool UIOverlay::update()
{
    ImDrawData *imDrawData       = ImGui::GetDrawData();
    bool        updateCmdBuffers = false;

    if (!imDrawData)
    {
        return false;
    };

    // Note: Alignment is done inside buffer creation
    VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize indexBufferSize  = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

    // Update buffers only if vertex or index count has been changed compared to current buffer size
    if ((vertexBufferSize == 0) || (indexBufferSize == 0))
    {
        return false;
    }

    // Vertex buffer
    if ((vertexBuffer == nullptr) || (vertexCount != imDrawData->TotalVtxCount))
    {
        vertexBuffer     = Buffer::create(deviceWrapper,
                                      vertexBufferSize,
                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        vertexCount      = imDrawData->TotalVtxCount;
        updateCmdBuffers = true;
    }

    // Index buffer
    if ((indexBuffer == nullptr) || (indexCount < imDrawData->TotalIdxCount))
    {
        indexBuffer      = Buffer::create(deviceWrapper,
                                     indexBufferSize,
                                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        indexCount       = imDrawData->TotalIdxCount;
        updateCmdBuffers = true;
    }

    // Upload data
    ImDrawVert *vtxDst = nullptr;
    ImDrawIdx * idxDst = nullptr;
    CALL_VK(vkMapMemory(deviceWrapper->logicalDevice,
                        vertexBuffer->getMemoryHandle(),
                        0,
                        vertexBufferSize,
                        0,
                        reinterpret_cast<void **>(&vtxDst)));
    CALL_VK(vkMapMemory(deviceWrapper->logicalDevice,
                        indexBuffer->getMemoryHandle(),
                        0,
                        indexBufferSize,
                        0,
                        reinterpret_cast<void **>(&idxDst)));

    for (int n = 0; n < imDrawData->CmdListsCount; n++)
    {
        const ImDrawList *cmd_list = imDrawData->CmdLists[n];
        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmd_list->VtxBuffer.Size;
        idxDst += cmd_list->IdxBuffer.Size;
    }

    // Flush to make writes visible to GPU
    vertexBuffer->flush();
    indexBuffer->flush();

    vkUnmapMemory(deviceWrapper->logicalDevice, vertexBuffer->getMemoryHandle());
    vkUnmapMemory(deviceWrapper->logicalDevice, indexBuffer->getMemoryHandle());

    return updateCmdBuffers;
}

void UIOverlay::draw(const VkCommandBuffer commandBuffer)
{
    ImDrawData *imDrawData   = ImGui::GetDrawData();
    int32_t     vertexOffset = 0;
    int32_t     indexOffset  = 0;

    if ((!imDrawData) || (imDrawData->CmdListsCount == 0))
    {
        return;
    }

    ImGuiIO &io = ImGui::GetIO();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle());
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout.handle(),
                            0,
                            1,
                            &descriptorSet,
                            0,
                            NULL);

    pushConstBlock.scale     = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    pushConstBlock.translate = glm::vec2(-1.0f);
    vkCmdPushConstants(commandBuffer,
                       pipelineLayout.handle(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0,
                       sizeof(PushConstBlock),
                       &pushConstBlock);

    VkDeviceSize offsets[1] = {0};
    VkBuffer     vertBuf    = vertexBuffer->getBufferHandle();
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertBuf, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBufferHandle(), 0, VK_INDEX_TYPE_UINT16);

    for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
    {
        const ImDrawList *cmd_list = imDrawData->CmdLists[i];
        for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
        {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
            VkRect2D         scissorRect;
            scissorRect.offset.x      = std::max((int32_t) (pcmd->ClipRect.x), 0);
            scissorRect.offset.y      = std::max((int32_t) (pcmd->ClipRect.y), 0);
            scissorRect.extent.width  = (uint32_t) (pcmd->ClipRect.z - pcmd->ClipRect.x);
            scissorRect.extent.height = (uint32_t) (pcmd->ClipRect.w - pcmd->ClipRect.y);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
            vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
            indexOffset += pcmd->ElemCount;
        }
        vertexOffset += cmd_list->VtxBuffer.Size;
    }
}

void UIOverlay::resize(uint32_t width, uint32_t height)
{
    ImGuiIO &io    = ImGui::GetIO();
    io.DisplaySize = ImVec2((float) (width), (float) (height));
}

void UIOverlay::freeResources()
{
    ImGui::DestroyContext();
}

bool UIOverlay::header(const char *caption)
{
    return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
}

bool UIOverlay::checkBox(const char *caption, bool *value)
{
    bool res = ImGui::Checkbox(caption, value);
    if (res)
    {
        updated = true;
    };
    return res;
}

bool UIOverlay::checkBox(const char *caption, int32_t *value)
{
    bool val = (*value == 1);
    bool res = ImGui::Checkbox(caption, &val);
    *value   = val;
    if (res)
    {
        updated = true;
    };
    return res;
}

bool UIOverlay::inputFloat(const char *caption, float *value, float step, uint32_t precision)
{
    bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
    if (res)
    {
        updated = true;
    };
    return res;
}

bool UIOverlay::sliderFloat(const char *caption, float *value, float min, float max)
{
    bool res = ImGui::SliderFloat(caption, value, min, max);
    if (res)
    {
        updated = true;
    };
    return res;
}

bool UIOverlay::sliderInt(const char *caption, int32_t *value, int32_t min, int32_t max)
{
    bool res = ImGui::SliderInt(caption, value, min, max);
    if (res)
    {
        updated = true;
    };
    return res;
}

bool UIOverlay::comboBox(const char *caption, int32_t *itemindex, std::vector<std::string> items)
{
    if (items.empty())
    {
        return false;
    }
    std::vector<const char *> charitems;
    charitems.reserve(items.size());
    for (size_t i = 0; i < items.size(); i++)
    {
        charitems.push_back(items[i].c_str());
    }
    uint32_t itemCount = static_cast<uint32_t>(charitems.size());
    bool     res       = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
    if (res)
    {
        updated = true;
    };
    return res;
}

bool UIOverlay::button(const char *caption)
{
    bool res = ImGui::Button(caption);
    if (res)
    {
        updated = true;
    };
    return res;
}

void UIOverlay::text(const char *formatstr, ...)
{
    va_list args;
    va_start(args, formatstr);
    ImGui::TextV(formatstr, args);
    va_end(args);
}
}        // namespace vks