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

#ifndef GAINVULKANSAMPLE_SAMPLE_07_HISTOGRAM_H
#define GAINVULKANSAMPLE_SAMPLE_07_HISTOGRAM_H

#include "VulkanInitializers.hpp"
#include <VulkanContextBase.h>
#include <VulkanResources.h>
#include <array>
#include <vector>

class Sample_07_Histogram : public VulkanContextBase
{
  private:
    // Images
    std::unique_ptr<Image> mYImage;
    std::unique_ptr<Image> mUImage;
    std::unique_ptr<Image> mVImage;

    std::unique_ptr<Buffer> mStorageBuffer;

    YUVSinglePassImage mYPlane;
    YUVSinglePassImage mUPlane;
    YUVSinglePassImage mVPlane;

    std::vector<jobject> mGlobalBitmaps;

    const char *mComputeShaderPath;

    // 计算管线相关资源
    struct
    {
        uint32_t queueFamilyIndex;        // Used to check if compute and graphics queue families differ
                                          // and require additional barriers
        VkQueue queue;                    // Separate queue for compute commands (queue family may differ from the
                                          // one used for graphics)
        VkCommandPool commandPool;        // Use a separate command pool (queue family may differ from
                                          // the one used for graphics)
        VkCommandBuffer
                              commandBuffer;              // Command buffer storing the dispatch commands and barriers
        VkSemaphore           semaphore;                  // Execution dependency between compute & graphic submission
        VkDescriptorSetLayout descriptorSetLayout;        // Compute shader binding layout
        VkDescriptorSet       descriptorSet;              // Compute shader bindings
        VkPipelineLayout      pipelineLayout;             // Layout of the compute pipeline
        VkPipeline
            pipelineYuvToRgba;        // Compute pipeline for N-Body velocity calculation (1st pass)
    } compute;

    VulkanSemaphore
        mGraphicsSemaphore;        // Execution dependency between compute & graphic submission

    void prepareGraphics();

    void prepareCompute();

    void buildComputeCommandBuffer();

    void prepareSynchronizationPrimitives();

    void updateUniformBuffers();

    void setupDescriptorSetLayout();

    void setupDescriptorPool();

    void updateTexture(JNIEnv *jniEnv);

  public:
    Sample_07_Histogram() :
        VulkanContextBase("shaders/shader_07_histogram.vert.spv",
                          "shaders/shader_07_histogram.frag.spv"),
        mComputeShaderPath("shaders/histogram.comp.spv"),
        mGraphicsSemaphore(VK_NULL_HANDLE)
    {
        settings.overlay = false;
    }

    virtual void prepare(JNIEnv *env) override;

    virtual void preparePipelines() override;

    virtual void setupDescriptorSet();

    virtual void buildCommandBuffers() override;

    virtual void prepareUniformBuffers();

    virtual void draw();

    void setYUVImage(uint8_t *yData, uint8_t *uData, uint8_t *vData, uint32_t w, uint32_t h,
                     uint32_t yStride, uint32_t uStride, uint32_t vStride, uint32_t uPixelStride,
                     uint32_t vPixelStride, uint32_t orientation);

    void prepareImages(JNIEnv *env);

    virtual void unInit(JNIEnv *env) override;

    ~Sample_07_Histogram();
};

#endif        // GAINVULKANSAMPLE_SAMPLE_07_HISTOGRAM_H
