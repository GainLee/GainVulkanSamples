/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 by Gain
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef GAINVULKANSAMPLE_VULKANRESOURCES_H
#define GAINVULKANSAMPLE_VULKANRESOURCES_H

#include <android/bitmap.h>
#include <android/hardware_buffer_jni.h>
#include <jni.h>

#include <android/asset_manager.h>
#include <gli/gli.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "../util/VulkanRAIIUtil.h"
#include "VulkanDeviceWrapper.hpp"

namespace gain
{
class Buffer
{
  public:
    // Create a buffer and allocate the memory.
    static std::unique_ptr<Buffer> create(
        const std::shared_ptr<vks::VulkanDeviceWrapper> deviceWrapper, uint32_t size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    // Prefer Buffer::create
    Buffer(const std::shared_ptr<vks::VulkanDeviceWrapper> deviceWrapper, uint32_t size);

    // Set the buffer content from the data. The buffer must be created with host-visible and
    // host-coherent properties.
    bool copyFrom(const void *data);

    VkBuffer getBufferHandle() const
    {
        return mBuffer.handle();
    }

    VkDeviceMemory getMemoryHandle() const
    {
        return mMemory.handle();
    }

    VkDescriptorBufferInfo getDescriptor() const
    {
        return {mBuffer.handle(), 0, mSize};
    }

    VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

  private:
    bool initialize(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    const std::shared_ptr<vks::VulkanDeviceWrapper> mContext;
    VkQueue                                         mVkQueue;
    uint32_t                                        mSize;

    // Managed handles
    VulkanBuffer       mBuffer;
    VulkanDeviceMemory mMemory;
};

class Image
{
  public:
    struct ImageBasicInfo
    {
        VkImageType       imageType = VK_IMAGE_TYPE_2D;
        VkFormat          format    = VK_FORMAT_R8G8B8A8_UNORM;
        uint32_t          mipLevels = 1;
        VkImageLayout     layout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkExtent3D        extent;
        uint32_t          arrayLayers = 1;
        VkImageUsageFlags usage       = VK_IMAGE_USAGE_SAMPLED_BIT;
    };

    // Create a image backed by device local memory. The layout is VK_IMAGE_LAYOUT_UNDEFINED
    // after the creation.
    static std::unique_ptr<Image> createDeviceLocal(
        const std::shared_ptr<vks::VulkanDeviceWrapper> deviceWrapper, VkQueue queue,
        const ImageBasicInfo &imageInfo);

    // Create a image backed by device local memory, and initialize the memory from a bitmap image.
    // The image is created with usage VK_IMAGE_USAGE_TRANSFER_DST_BIT and
    // VK_IMAGE_USAGE_SAMPLED_BIT as an input of shader. The layout is set to
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL after the creation.
    static std::unique_ptr<Image> createFromBitmap(
        const std::shared_ptr<vks::VulkanDeviceWrapper> deviceWrapper, VkQueue queue, JNIEnv *env,
        jobject bitmap, VkImageUsageFlags usage, VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED);

    // Create a cube image backed by device local memory, and initialize the memory from a bitmap
    // image. The image is created with usage VK_IMAGE_USAGE_TRANSFER_DST_BIT and
    // VK_IMAGE_USAGE_SAMPLED_BIT as an input of shader. The layout is set to
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL after the creation.
    static std::unique_ptr<Image> create3DImageFromBitmap(
        const std::shared_ptr<vks::VulkanDeviceWrapper> deviceWrapper, VkQueue queue, JNIEnv *env,
        jobject bitmap, VkImageUsageFlags usage, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    static std::unique_ptr<Image> createCubeMapFromFile(
        const std::shared_ptr<vks::VulkanDeviceWrapper> deviceWrapper, VkQueue queue, AAssetManager *asset,
        std::string filename, const ImageBasicInfo &imageInfo);

    // Put an image memory barrier for setting an image layout on the sub resource into the given
    // command buffer
    static void setImageLayout(
        VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    // Uses a fixed sub resource layout with first mip level and layer
    static void setImageLayout(
        VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask,
        VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    Image(const std::shared_ptr<vks::VulkanDeviceWrapper> deviceWrapper, VkQueue queue,
          const ImageBasicInfo &imageInfo);

    ~Image()
    {
        if (mBuffer != nullptr)
        {
            AHardwareBuffer_release(mBuffer);
        }

        if (mYMemory)
        {
            vkFreeMemory(mDeviceWrapper->logicalDevice, mYMemory, nullptr);
        }
        if (mUMemory)
        {
            vkFreeMemory(mDeviceWrapper->logicalDevice, mUMemory, nullptr);
        }
        if (mVMemory)
        {
            vkFreeMemory(mDeviceWrapper->logicalDevice, mVMemory, nullptr);
        }
        if (mSamplerYcbcrConversion)
        {
            //            vkDestroySamplerYcbcrConversion(mDeviceWrapper->logicalDevice, mSamplerYcbcrConversion, nullptr);
        }
    }

    uint32_t width() const
    {
        return mImageInfo.extent.width;
    }

    uint32_t height() const
    {
        return mImageInfo.extent.height;
    }

    VkImage getImageHandle() const
    {
        return mImage.handle();
    }

    VkImageView getImageViewHandle() const
    {
        return mImageView.handle();
    }

    VkSampler getSamplerHandle() const
    {
        return mSampler.handle();
    }

    AHardwareBuffer *getAHardwareBuffer()
    {
        return mBuffer;
    }

    // Copy the bytes to the image device memory. The image must be created with
    // VK_IMAGE_USAGE_TRANSFER_DST_BIT.
    bool setContentFromBytes(const void *data, uint32_t bufferSize, uint32_t stride);

    bool setYUVContentForYCbCrImage(const void *data, uint32_t size);

    // Copy the bitmap pixels to the image device memory. The image must be created with
    // VK_IMAGE_USAGE_TRANSFER_DST_BIT.
    bool setContentFromBitmap(JNIEnv *env, jobject bitmap);

    bool setCubemapData(const gli::texture_cube &texCube);

    VkDescriptorImageInfo getDescriptor() const
    {
        return {mSampler.handle(), mImageView.handle(), mImageInfo.layout};
    }

  private:
    // Initialization
    bool createDeviceLocalImage();

    bool createImageFromAHardwareBuffer(AHardwareBuffer *buffer);

    bool createSampler();

    bool createImageView();

    bool createSamplerYcbcrConversionInfo();

    bool isYUVFormat();

    // Context
    const std::shared_ptr<vks::VulkanDeviceWrapper> mDeviceWrapper;

    VkQueue mVkQueue;

    ImageBasicInfo mImageInfo;

    // The managed AHardwareBuffer handle. Only valid if the image is created from
    // Image::createFromAHardwareBuffer.
    AHardwareBuffer *mBuffer = nullptr;

    // Managed handles
    VulkanImage        mImage;
    VulkanDeviceMemory mMemory;
    VulkanSampler      mSampler;
    VulkanImageView    mImageView;

    // DeviceMemory for YUV
    VkDeviceMemory mYMemory = VK_NULL_HANDLE;
    VkDeviceMemory mUMemory = VK_NULL_HANDLE;
    VkDeviceMemory mVMemory = VK_NULL_HANDLE;

    VkSamplerYcbcrConversionKHR  mSamplerYcbcrConversion = VK_NULL_HANDLE;
    VkSamplerYcbcrConversionInfo mSamplerYcbcrConversionInfo;
};

}        // namespace gain

#endif        // GAINVULKANSAMPLE_VULKANRESOURCES_H
