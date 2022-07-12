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

#ifndef GAINVULKANSAMPLE_SAMPLE_11_YUVTEXTURE_VK_CONVERSION_H
#define GAINVULKANSAMPLE_SAMPLE_11_YUVTEXTURE_VK_CONVERSION_H

#include <VulkanContextBase.h>
#include <VulkanImageWrapper.h>
#include <array>

class Sample_11_YUVTexture_VK_Conversion : public VulkanContextBase
{
  private:
    // Images
    std::unique_ptr<Image> mYUVImage;

    struct YUVData
    {
        uint8_t *data;
        uint32_t w;
        uint32_t h;
    } mYUVData;

    void prepareSynchronizationPrimitives();

    void updateUniformBuffers();

    void setupDescriptorSetLayout();

    void setupDescriptorPool();

    void updateTexture();

  public:
    Sample_11_YUVTexture_VK_Conversion() :
        VulkanContextBase("shaders/shader_11_yuv_vk_conversion.vert.spv",
                          "shaders/shader_11_yuv_vk_conversion.frag.spv")
    {}

    virtual void prepare(JNIEnv *env) override;

    virtual void preparePipelines() override;

    virtual void setupDescriptorSet();

    virtual void buildCommandBuffers() override;

    virtual void prepareUniformBuffers();

    virtual void draw();

    void setYUVImage(uint8_t *data, uint32_t w, uint32_t h);

    void prepareYUVImage();

    ~Sample_11_YUVTexture_VK_Conversion();
};

#endif        //GAINVULKANSAMPLE_SAMPLE_11_YUVTEXTURE_VK_CONVERSION_H
