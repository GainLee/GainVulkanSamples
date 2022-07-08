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

#ifndef GAINVULKANSAMPLE_SAMPLE_06_MULTILUT_H
#define GAINVULKANSAMPLE_SAMPLE_06_MULTILUT_H

#include "LutFilter.h"
#include "VulkanInitializers.hpp"
#include <VulkanContextBase.h>
#include <VulkanResources.h>
#include <array>
#include <vector>

class Sample_06_MultiLUT : public VulkanContextBase
{
  private:
    // Images
    std::unique_ptr<Image>              mYImage;
    std::unique_ptr<Image>              mUImage;
    std::unique_ptr<Image>              mVImage;
    std::vector<std::unique_ptr<Image>> mLUTImages;

    std::unique_ptr<Image> mSelectedFilter;

    std::array<YUVSinglePassImage, 3> mYUVImages;

    std::vector<jobject> mGlobalBitmaps;

    struct
    {
        uint32_t itemWidth;
        uint32_t startIndex;
        uint32_t drawCount;
        uint32_t offset;
    } mLUTProperty;

    // Uniform buffer block object
    std::unique_ptr<Buffer> mLutUniformBuffer;

    struct
    {
        glm::mat4 projectionMatrix;
        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
    } lutUBOVS;

    LutPushConstantData mLutPushConstantData;

    std::vector<LutFilter> mFilters;

    void prepareSynchronizationPrimitives();

    void updateUniformBuffers();

    void updateLutMatrix();

    void setupDescriptorSetLayout();

    void setupDescriptorPool();

    void updateTexture(JNIEnv *jniEnv);

  public:
    Sample_06_MultiLUT() :
        VulkanContextBase("shaders/shader_06_multi_lut.vert.spv",
                          "shaders/shader_06_multi_lut.frag.spv")
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
                     uint32_t yStride, uint32_t uStride, uint32_t vStride, uint32_t orientation);

    void setLUTImages(JNIEnv *jniEnv, jobjectArray jbitmaps);

    void updateLUTs(JNIEnv *jniEnv, uint32_t itemWidth, uint32_t startIndex, uint32_t drawCount,
                    uint32_t offset);

    void updateSelectedIndex(JNIEnv *jniEnv, uint32_t index);

    void prepareImages(JNIEnv *env);

    virtual void unInit(JNIEnv *env) override;

    ~Sample_06_MultiLUT();
};

#endif        // GAINVULKANSAMPLE_SAMPLE_06_MULTILUT_H
