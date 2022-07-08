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

#ifndef GAINVULKANSAMPLE_LUTFILTER_H
#define GAINVULKANSAMPLE_LUTFILTER_H

#include <VulkanResources.h>

using namespace gain;

class VulkanContextBase;

struct LutPushConstantData
{
    float_t itemWidth;
    float_t windowWidth;
};

struct SpecializationData
{
    float_t LUT_ITEM_INDEX;
};

class LutFilter
{
  private:
    VulkanContextBase *context;

    std::vector<VkPipelineShaderStageCreateInfo> shaders;

    VulkanDescriptorPool      descriptorPool;
    VulkanDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet           descriptorSet;
    VulkanPipelineLayout      pipelineLayout;
    VulkanPipeline            pipeline;

    // Vertex buffer and attributes
    std::unique_ptr<Buffer> mVerticesBuffer;

    void prepareResource(std::vector<VkDescriptorImageInfo> yuvDescriptors,
                         VkDescriptorImageInfo lutDescriptor, VkDescriptorBufferInfo bufDescriptor);

    void preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass);

    void prepareVertices(bool useStagingBuffers, const void *data, size_t bufSize);

  public:
    SpecializationData specializationData;

    LutFilter(VulkanContextBase *c);

    LutFilter(const LutFilter &filter);

    void prepare(std::vector<VkDescriptorImageInfo> yuvDescriptors,
                 VkDescriptorImageInfo lutDescriptor, VkDescriptorBufferInfo bufDescriptor,
                 const VkPipelineCache pipelineCache, const VkRenderPass renderPass);

    void draw(const VkCommandBuffer commandBuffer, float_t itemWidth, float_t windowWidth);

    ~LutFilter();
};

#endif        // GAINVULKANSAMPLE_LUTFILTER_H
