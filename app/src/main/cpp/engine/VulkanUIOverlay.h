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

#pragma once

#include <assert.h>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include <vulkan_wrapper.h>

#include "../util/glm/glm.hpp"
#include "../util/imgui/imgui.h"
#include "VulkanImageWrapper.h"
#include "VulkanBufferWrapper.h"
#include <android/asset_manager.h>

using namespace vks;

namespace vks
{
class UIOverlay
{
  public:
    std::shared_ptr<VulkanDeviceWrapper> deviceWrapper;
    VkQueue                              queue;
    uint32_t                             screenDensity;
    AAssetManager *                      assetManager;

    VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    uint32_t              subpass              = 0;

    std::unique_ptr<Buffer> vertexBuffer;
    std::unique_ptr<Buffer> indexBuffer;
    int32_t                 vertexCount = 0;
    int32_t                 indexCount  = 0;

    std::vector<VkPipelineShaderStageCreateInfo> shaders;

    VulkanDescriptorPool      descriptorPool;
    VulkanDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet           descriptorSet;
    VulkanPipelineLayout      pipelineLayout;
    VulkanPipeline            pipeline;

    std::unique_ptr<Image> fontImage;

    struct PushConstBlock
    {
        glm::vec2 scale;
        glm::vec2 translate;
    } pushConstBlock;

    bool  visible = true;
    bool  updated = false;
    float scale   = 1.0f;

    UIOverlay() :
        descriptorPool(VK_NULL_HANDLE), descriptorSetLayout(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), pipeline(VK_NULL_HANDLE){};
    ~UIOverlay(){};

    void init();

    void initRAIIObjects();

    void preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass);
    void prepareResources();

    bool update();
    void draw(const VkCommandBuffer commandBuffer);
    void resize(uint32_t width, uint32_t height);

    void freeResources();

    bool header(const char *caption);
    bool checkBox(const char *caption, bool *value);
    bool checkBox(const char *caption, int32_t *value);
    bool inputFloat(const char *caption, float *value, float step, uint32_t precision);
    bool sliderFloat(const char *caption, float *value, float min, float max);
    bool sliderInt(const char *caption, int32_t *value, int32_t min, int32_t max);
    bool comboBox(const char *caption, int32_t *itemindex, std::vector<std::string> items);
    bool button(const char *caption);
    void text(const char *formatstr, ...);
};
}        // namespace vks