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

#ifndef GAINVULKANSAMPLE_SAMPLE_04_YUVTEXTURE_H
#define GAINVULKANSAMPLE_SAMPLE_04_YUVTEXTURE_H

#include <VulkanContextBase.h>
#include <VulkanResources.h>
#include <array>

class Sample_04_YUVTexture : public VulkanContextBase
{
  private:
    // Images
    std::unique_ptr<Image> mYImage;
    std::unique_ptr<Image> mUImage;
    std::unique_ptr<Image> mVImage;

    std::array<YUVSinglePassImage, 3> mYUVImages;

    void prepareSynchronizationPrimitives();

    void updateUniformBuffers();

    void setupDescriptorSetLayout();

    void setupDescriptorPool();

    void updateTexture();

  public:
    Sample_04_YUVTexture() :
        VulkanContextBase("shaders/shader_04_yuvtexture.vert.spv",
                          "shaders/shader_04_yuvtexture.frag.spv")
    {}

    virtual void prepare(JNIEnv *env) override;

    virtual void preparePipelines() override;

    virtual void setupDescriptorSet();

    virtual void buildCommandBuffers() override;

    virtual void prepareUniformBuffers();

    virtual void draw();

    void setYUVImage(uint8_t *yData, uint8_t *uData, uint8_t *vData, uint32_t w, uint32_t h,
                     uint32_t yStride, uint32_t uStride, uint32_t vStride, uint32_t orientation);

    void prepareYUVImage();

    ~Sample_04_YUVTexture();
};

#endif        // GAINVULKANSAMPLE_SAMPLE_04_YUVTEXTURE_H
