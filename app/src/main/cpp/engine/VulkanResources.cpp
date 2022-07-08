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

#include "VulkanResources.h"

#include <android/bitmap.h>
#include <android/hardware_buffer_jni.h>
#include <jni.h>

#include <LogUtil.h>
#include <memory>
#include <optional>
#include <vector>
#include <vulkan_wrapper.h>

#include <VulkanInitializers.hpp>

#include "../util/VulkanRAIIUtil.h"

namespace gain
{
std::unique_ptr<Buffer> Buffer::create(const std::shared_ptr<vks::VulkanDeviceWrapper> context,
                                       uint32_t size, VkBufferUsageFlags usage,
                                       VkMemoryPropertyFlags properties)
{
    auto       buffer  = std::make_unique<Buffer>(context, size);
    const bool success = buffer->initialize(usage, properties);
    return success ? std::move(buffer) : nullptr;
}

Buffer::Buffer(const std::shared_ptr<vks::VulkanDeviceWrapper> context, uint32_t size) :
    mContext(context), mSize(size), mBuffer(context->logicalDevice), mMemory(context->logicalDevice)
{}

bool Buffer::initialize(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    // Create buffer
    const VkBufferCreateInfo bufferCreateInfo = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = mSize,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    CALL_VK(vkCreateBuffer(mContext->logicalDevice, &bufferCreateInfo, nullptr, mBuffer.pHandle()));

    // Allocate memory for the buffer
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(mContext->logicalDevice, mBuffer.handle(), &memoryRequirements);
    const auto memoryTypeIndex =
        mContext->getMemoryType(memoryRequirements.memoryTypeBits, properties);
    const VkMemoryAllocateInfo allocateInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = nullptr,
        .allocationSize  = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex,
    };
    CALL_VK(vkAllocateMemory(mContext->logicalDevice, &allocateInfo, nullptr, mMemory.pHandle()));

    vkBindBufferMemory(mContext->logicalDevice, mBuffer.handle(), mMemory.handle(), 0);

    vks::debug::setDeviceMemoryName(mContext->logicalDevice, mMemory.handle(), "VulkanResources-Buffer::initialize-mMemory");

    return true;
}

bool Buffer::copyFrom(const void *data)
{
    void *bufferData = nullptr;
    CALL_VK(vkMapMemory(mContext->logicalDevice, mMemory.handle(), 0, mSize, 0, &bufferData));
    memcpy(bufferData, data, mSize);
    vkUnmapMemory(mContext->logicalDevice, mMemory.handle());
    return true;
}

/**
 * Flush a memory range of the buffer to make it visible to the device
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
 * complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the flush call
 */
VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset)
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory              = mMemory.handle();
    mappedRange.offset              = offset;
    mappedRange.size                = size;
    return vkFlushMappedMemoryRanges(mContext->logicalDevice, 1, &mappedRange);
}

std::unique_ptr<Image> Image::createDeviceLocal(
    const std::shared_ptr<vks::VulkanDeviceWrapper> context, VkQueue queue, const ImageBasicInfo &imageInfo)
{
    auto image   = std::make_unique<Image>(context, queue, imageInfo);
    bool success = image->createDeviceLocalImage();
    if (image->isYUVFormat())
    {
        success = success && image->createSamplerYcbcrConversionInfo();
    }
    success = success && image->createImageView();
    // Sampler is only needed for sampled images.
    if (imageInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        success = success && image->createSampler();
    }
    return success ? std::move(image) : nullptr;
}

std::unique_ptr<Image> Image::createFromBitmap(
    const std::shared_ptr<vks::VulkanDeviceWrapper> context, VkQueue queue, JNIEnv *env,
    jobject bitmap, VkImageUsageFlags usage, VkImageLayout layout)
{
    // Get bitmap info
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) != ANDROID_BITMAP_RESULT_SUCCESS)
    {
        LOGCATE("Image::createFromBitmap: Failed to AndroidBitmap_getInfo");
        return nullptr;
    }

    ImageBasicInfo imageInfo = {
        extent: {info.width, info.height, 1},
        usage: usage,
        format: VK_FORMAT_R8G8B8A8_UNORM,
        layout: layout
    };

    // Create device local image
    auto image = Image::createDeviceLocal(context, queue, imageInfo);
    if (image == nullptr)
        return nullptr;

    // Set content from bitmap
    const bool success = image->setContentFromBitmap(env, bitmap);
    return success ? std::move(image) : nullptr;
}

std::unique_ptr<Image> Image::createCubeMapFromFile(
    const std::shared_ptr<vks::VulkanDeviceWrapper> deviceWrapper, VkQueue queue, AAssetManager *assetMgr,
    std::string filename, const ImageBasicInfo &info)
{
    // Textures are stored inside the apk on Android (compressed)
    // So they need to be loaded via the asset manager
    AAsset *asset = AAssetManager_open(assetMgr, filename.c_str(), AASSET_MODE_STREAMING);
    if (!asset)
    {
        LOGCATE("Could not load texture %s", filename.c_str());
        exit(-1);
    }
    size_t size = AAsset_getLength(asset);
    assert(size > 0);

    void *textureData = malloc(size);
    AAsset_read(asset, textureData, size);
    AAsset_close(asset);

    gli::texture_cube texCube(gli::load((const char *) textureData, size));

    free(textureData);

    assert(!texCube.empty());

    ImageBasicInfo imgInfo(info);
    imgInfo.extent      = {static_cast<uint32_t>(texCube.extent().x), static_cast<uint32_t>(texCube.extent().y), 1};
    imgInfo.mipLevels   = static_cast<uint32_t>(texCube.levels());
    imgInfo.arrayLayers = 6;

    // Create device local image
    auto image = Image::createDeviceLocal(deviceWrapper, queue, imgInfo);
    if (image == nullptr)
        return nullptr;

    bool result = image->setCubemapData(texCube);

    return result ? std::move(image) : nullptr;
}

std::unique_ptr<Image> Image::create3DImageFromBitmap(
    const std::shared_ptr<vks::VulkanDeviceWrapper> deviceWrapper, VkQueue queue, JNIEnv *env,
    jobject bitmap, VkImageUsageFlags usage, VkImageLayout layout)
{
    // Get bitmap info
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) != ANDROID_BITMAP_RESULT_SUCCESS)
    {
        LOGCATE("Image::createFromBitmap: Failed to AndroidBitmap_getInfo");
        return nullptr;
    }

    // Create device local image
    uint32_t       cubeSize  = std::min(info.width, info.height);
    ImageBasicInfo imageInfo = {
        imageType: VK_IMAGE_TYPE_3D,
        extent: {cubeSize, cubeSize, cubeSize},
        usage: usage,
        format: VK_FORMAT_R8G8B8A8_UNORM,
        layout: layout
    };
    auto image = Image::createDeviceLocal(deviceWrapper, queue, imageInfo);
    if (image == nullptr)
        return nullptr;

    // Set content from bitmap
    const bool success = image->setContentFromBitmap(env, bitmap);
    return success ? std::move(image) : nullptr;
}

bool Image::createDeviceLocalImage()
{
    // Create an image
    VkImageCreateInfo imageCreateInfo = {
        .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .imageType             = mImageInfo.imageType,
        .format                = mImageInfo.format,
        .extent                = mImageInfo.extent,
        .mipLevels             = mImageInfo.mipLevels,
        .arrayLayers           = mImageInfo.arrayLayers,
        .samples               = VK_SAMPLE_COUNT_1_BIT,
        .tiling                = VK_IMAGE_TILING_OPTIMAL,
        .usage                 = mImageInfo.usage,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (mImageInfo.arrayLayers == 6)
    {
        // This flag is required for cube map images
        imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    if (isYUVFormat())
    {
        imageCreateInfo.flags = VK_IMAGE_CREATE_DISJOINT_BIT;
    }
    CALL_VK(
        vkCreateImage(mDeviceWrapper->logicalDevice, &imageCreateInfo, nullptr, mImage.pHandle()));

    // Allocate device memory
    if (mImageInfo.format == VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM)
    {
        VkImagePlaneMemoryRequirementsInfo imagePlaneMemoryRequirementsInfo = {VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO};

        VkImageMemoryRequirementsInfo2 imageMemoryRequirementsInfo2 = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};
        imageMemoryRequirementsInfo2.pNext                          = &imagePlaneMemoryRequirementsInfo;
        imageMemoryRequirementsInfo2.image                          = mImage.handle();

        // Get memory requirement for each plane
        imagePlaneMemoryRequirementsInfo.planeAspect    = VK_IMAGE_ASPECT_PLANE_0_BIT;
        VkMemoryRequirements2 memoryRequirements2Plane0 = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
        vkGetImageMemoryRequirements2(mDeviceWrapper->logicalDevice, &imageMemoryRequirementsInfo2, &memoryRequirements2Plane0);

        imagePlaneMemoryRequirementsInfo.planeAspect    = VK_IMAGE_ASPECT_PLANE_1_BIT;
        VkMemoryRequirements2 memoryRequirements2Plane1 = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
        vkGetImageMemoryRequirements2(mDeviceWrapper->logicalDevice, &imageMemoryRequirementsInfo2, &memoryRequirements2Plane1);

        imagePlaneMemoryRequirementsInfo.planeAspect    = VK_IMAGE_ASPECT_PLANE_2_BIT;
        VkMemoryRequirements2 memoryRequirements2Plane2 = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
        vkGetImageMemoryRequirements2(mDeviceWrapper->logicalDevice, &imageMemoryRequirementsInfo2, &memoryRequirements2Plane2);

        // Allocate memory for each plane
        VkMemoryAllocateInfo memoryAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};

        memoryAllocateInfo.allocationSize = memoryRequirements2Plane0.memoryRequirements.size;
        vkAllocateMemory(mDeviceWrapper->logicalDevice, &memoryAllocateInfo, nullptr, &mYMemory);

        memoryAllocateInfo.allocationSize = memoryRequirements2Plane1.memoryRequirements.size;
        vkAllocateMemory(mDeviceWrapper->logicalDevice, &memoryAllocateInfo, nullptr, &mUMemory);

        memoryAllocateInfo.allocationSize = memoryRequirements2Plane2.memoryRequirements.size;
        vkAllocateMemory(mDeviceWrapper->logicalDevice, &memoryAllocateInfo, nullptr, &mVMemory);

        // Bind memory for each plane
        VkBindImagePlaneMemoryInfo bindImagePlaneMemoryInfoY{VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO};
        bindImagePlaneMemoryInfoY.planeAspect = VK_IMAGE_ASPECT_PLANE_0_BIT;
        VkBindImageMemoryInfo bindImageMemoryInfoY{VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO};
        bindImageMemoryInfoY.pNext  = &bindImagePlaneMemoryInfoY;
        bindImageMemoryInfoY.image  = mImage.handle();
        bindImageMemoryInfoY.memory = mYMemory;

        VkBindImagePlaneMemoryInfo bindImagePlaneMemoryInfoU{VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO};
        bindImagePlaneMemoryInfoU.planeAspect = VK_IMAGE_ASPECT_PLANE_1_BIT;
        VkBindImageMemoryInfo bindImageMemoryInfoU{VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO};
        bindImageMemoryInfoU.pNext  = &bindImagePlaneMemoryInfoU;
        bindImageMemoryInfoU.image  = mImage.handle();
        bindImageMemoryInfoU.memory = mUMemory;

        VkBindImagePlaneMemoryInfo bindImagePlaneMemoryInfoV{VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO};
        bindImagePlaneMemoryInfoV.planeAspect = VK_IMAGE_ASPECT_PLANE_2_BIT;
        VkBindImageMemoryInfo bindImageMemoryInfoV{VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO};
        bindImageMemoryInfoV.pNext  = &bindImagePlaneMemoryInfoV;
        bindImageMemoryInfoV.image  = mImage.handle();
        bindImageMemoryInfoV.memory = mVMemory;

        std::vector<VkBindImageMemoryInfo> bindImageMemoryInfos = {bindImageMemoryInfoY, bindImageMemoryInfoU, bindImageMemoryInfoV};

        vkBindImageMemory2(mDeviceWrapper->logicalDevice, static_cast<uint32_t>(bindImageMemoryInfos.size()), bindImageMemoryInfos.data());
    }
    else
    {
        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(
            mDeviceWrapper->logicalDevice, mImage.handle(), &memoryRequirements);
        uint32_t                   memoryTypeIndex = mDeviceWrapper->getMemoryType(memoryRequirements.memoryTypeBits,
                                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        const VkMemoryAllocateInfo allocateInfo    = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = nullptr,
            .allocationSize  = memoryRequirements.size,
            .memoryTypeIndex = memoryTypeIndex,
        };
        CALL_VK(
            vkAllocateMemory(mDeviceWrapper->logicalDevice, &allocateInfo, nullptr, mMemory.pHandle()));
        vkBindImageMemory(mDeviceWrapper->logicalDevice, mImage.handle(), mMemory.handle(), 0);
    }
    return true;
}

bool Image::setYUVContentForYCbCrImage(const void *data, uint32_t size)
{
    // Allocate staging buffers
    auto stagingBuffer =
        Buffer::create(mDeviceWrapper,
                       size,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    const bool success = stagingBuffer->copyFrom(data);
    assert(success);

    // Copy buffer to image
    VulkanCommandBuffer copyCommand(mDeviceWrapper->logicalDevice, mDeviceWrapper->commandPool);
    assert(mDeviceWrapper->beginSingleTimeCommand(copyCommand.pHandle()));

    // Set layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to prepare for buffer-image copy
    /*VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel            = 0;
    subresourceRange.levelCount              = mImageInfo.mipLevels;
    subresourceRange.layerCount              = mImageInfo.arrayLayers;
    setImageLayout(copyCommand.handle(), mImage.handle(),
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);*/

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask              = VK_IMAGE_ASPECT_PLANE_0_BIT;
    subresourceRange.baseMipLevel            = 0;
    subresourceRange.levelCount              = mImageInfo.mipLevels;
    subresourceRange.layerCount              = mImageInfo.arrayLayers;
    setImageLayout(copyCommand.handle(), mImage.handle(),
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
    setImageLayout(copyCommand.handle(), mImage.handle(),
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_PLANE_2_BIT;
    setImageLayout(copyCommand.handle(), mImage.handle(),
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    VkBufferImageCopy bufferCopyRegions[3];
    bufferCopyRegions[0].imageSubresource  = {VK_IMAGE_ASPECT_PLANE_0_BIT, 0, 0, 1};
    bufferCopyRegions[0].imageOffset       = {0, 0, 0};
    bufferCopyRegions[0].imageExtent       = mImageInfo.extent;
    bufferCopyRegions[0].bufferOffset      = 0;
    bufferCopyRegions[0].bufferRowLength   = mImageInfo.extent.width;
    bufferCopyRegions[0].bufferImageHeight = mImageInfo.extent.height;
    // the Cb component is half the height and width
    bufferCopyRegions[1].imageOffset       = {0, 0, 0};
    bufferCopyRegions[1].imageExtent       = {mImageInfo.extent.width / 2, mImageInfo.extent.height / 2, 1};
    bufferCopyRegions[1].imageSubresource  = {VK_IMAGE_ASPECT_PLANE_1_BIT, 0, 0, 1};
    bufferCopyRegions[1].bufferOffset      = mImageInfo.extent.width * mImageInfo.extent.height;
    bufferCopyRegions[1].bufferRowLength   = mImageInfo.extent.width / 2;
    bufferCopyRegions[1].bufferImageHeight = mImageInfo.extent.height / 2;
    // the Cr component is half the height and width
    bufferCopyRegions[2].imageOffset       = {0, 0, 0};
    bufferCopyRegions[2].imageExtent       = {mImageInfo.extent.width / 2, mImageInfo.extent.height / 2, 1};
    bufferCopyRegions[2].imageSubresource  = {VK_IMAGE_ASPECT_PLANE_2_BIT, 0, 0, 1};
    bufferCopyRegions[2].bufferOffset      = mImageInfo.extent.width * mImageInfo.extent.height + mImageInfo.extent.width * mImageInfo.extent.height / 4;
    bufferCopyRegions[2].bufferRowLength   = mImageInfo.extent.width / 2;
    bufferCopyRegions[2].bufferImageHeight = mImageInfo.extent.height / 2;

    vkCmdCopyBufferToImage(copyCommand.handle(),
                           stagingBuffer->getBufferHandle(),
                           mImage.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           3,
                           bufferCopyRegions);

    if (mImageInfo.layout != VK_IMAGE_LAYOUT_UNDEFINED && mImageInfo.layout != VK_IMAGE_LAYOUT_PREINITIALIZED)
    {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel            = 0;
        subresourceRange.levelCount              = mImageInfo.mipLevels;
        subresourceRange.layerCount              = mImageInfo.arrayLayers;
        setImageLayout(copyCommand.handle(), mImage.handle(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mImageInfo.layout, subresourceRange);
    }

    mDeviceWrapper->endAndSubmitSingleTimeCommand(copyCommand.handle(), mVkQueue, false);
    return true;
}

bool Image::setContentFromBytes(const void *data, uint32_t bufferSize, uint32_t stride)
{
    // Allocate a staging buffer
    auto stagingBuffer =
        Buffer::create(mDeviceWrapper,
                       bufferSize,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vks::debug::setDeviceMemoryName(mDeviceWrapper->logicalDevice, stagingBuffer->getMemoryHandle(), "VulkanResources-Image::setContentFromBytes-stagingBuffer");

    // Copy bitmap pixels to the buffer memory
    const bool success = stagingBuffer->copyFrom(data);
    assert(success);

    // Copy buffer to image
    VulkanCommandBuffer copyCommand(mDeviceWrapper->logicalDevice, mDeviceWrapper->commandPool);
    assert(mDeviceWrapper->beginSingleTimeCommand(copyCommand.pHandle()));

    // Set layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to prepare for buffer-image copy
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel            = 0;
    subresourceRange.levelCount              = mImageInfo.mipLevels;
    subresourceRange.layerCount              = mImageInfo.arrayLayers;
    setImageLayout(copyCommand.handle(), mImage.handle(),
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    const VkBufferImageCopy bufferImageCopy = {
        .bufferOffset      = 0,
        .bufferRowLength   = stride,
        .bufferImageHeight = mImageInfo.extent.depth == 1 ? mImageInfo.extent.height : mImageInfo.extent.height * mImageInfo.extent.height,
        .imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .imageOffset       = {0, 0, 0},
        .imageExtent       = mImageInfo.extent,
    };
    vkCmdCopyBufferToImage(copyCommand.handle(),
                           stagingBuffer->getBufferHandle(),
                           mImage.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &bufferImageCopy);

    if (mImageInfo.layout != VK_IMAGE_LAYOUT_UNDEFINED && mImageInfo.layout != VK_IMAGE_LAYOUT_PREINITIALIZED)
    {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel            = 0;
        subresourceRange.levelCount              = mImageInfo.mipLevels;
        subresourceRange.layerCount              = mImageInfo.arrayLayers;
        setImageLayout(copyCommand.handle(), mImage.handle(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mImageInfo.layout, subresourceRange);
    }

    mDeviceWrapper->endAndSubmitSingleTimeCommand(copyCommand.handle(), mVkQueue, false);
    return true;
}

bool Image::setCubemapData(const gli::texture_cube &texCube)
{
    auto stagingBuffer =
        Buffer::create(mDeviceWrapper,
                       texCube.size(),
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vks::debug::setDeviceMemoryName(mDeviceWrapper->logicalDevice, stagingBuffer->getMemoryHandle(), "VulkanResources-Image::createCubeMapFromFile-stagingBuffer");

    const bool success = stagingBuffer->copyFrom(texCube.data());
    assert(success);

    // Copy buffer to image
    VulkanCommandBuffer copyCommand(mDeviceWrapper->logicalDevice, mDeviceWrapper->commandPool);
    assert(mDeviceWrapper->beginSingleTimeCommand(copyCommand.pHandle()));

    // Set layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to prepare for buffer-image copy
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel            = 0;
    subresourceRange.levelCount              = mImageInfo.mipLevels;
    subresourceRange.layerCount              = mImageInfo.arrayLayers;
    setImageLayout(copyCommand.handle(), mImage.handle(),
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    // Setup buffer copy regions for each face including all of it's miplevels
    std::vector<VkBufferImageCopy> bufferCopyRegions;
    size_t                         offset = 0;

    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t level = 0; level < mImageInfo.mipLevels; level++)
        {
            VkBufferImageCopy bufferCopyRegion               = {};
            bufferCopyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel       = level;
            bufferCopyRegion.imageSubresource.baseArrayLayer = face;
            bufferCopyRegion.imageSubresource.layerCount     = 1;
            bufferCopyRegion.imageExtent.width               = static_cast<uint32_t>(texCube[face][level].extent().x);
            bufferCopyRegion.imageExtent.height              = static_cast<uint32_t>(texCube[face][level].extent().y);
            bufferCopyRegion.imageExtent.depth               = 1;
            bufferCopyRegion.bufferOffset                    = offset;

            bufferCopyRegions.push_back(bufferCopyRegion);

            // Increase offset into staging buffer for next level / face
            offset += texCube[face][level].size();
        }
    }

    vkCmdCopyBufferToImage(copyCommand.handle(),
                           stagingBuffer->getBufferHandle(),
                           mImage.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(bufferCopyRegions.size()),
                           bufferCopyRegions.data());

    if (mImageInfo.layout != VK_IMAGE_LAYOUT_UNDEFINED && mImageInfo.layout != VK_IMAGE_LAYOUT_PREINITIALIZED)
    {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel            = 0;
        subresourceRange.levelCount              = mImageInfo.mipLevels;
        subresourceRange.layerCount              = mImageInfo.arrayLayers;
        setImageLayout(copyCommand.handle(), mImage.handle(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mImageInfo.layout, subresourceRange);
    }

    mDeviceWrapper->endAndSubmitSingleTimeCommand(copyCommand.handle(), mVkQueue, false);
    return true;
}

bool Image::setContentFromBitmap(JNIEnv *env, jobject bitmap)
{
    // Get bitmap info
    AndroidBitmapInfo info;
    assert(AndroidBitmap_getInfo(env, bitmap, &info) == ANDROID_BITMAP_RESULT_SUCCESS);
    // We don't assert these in cube image
    if (mImageInfo.extent.depth == 1)
    {
        assert(info.width == mImageInfo.extent.width);
        assert(info.height == mImageInfo.extent.height);
    }
    assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888);
    assert(info.stride % 4 == 0);

    // Allocate a staging buffer
    const uint32_t bufferSize = info.stride * info.height;
    auto           stagingBuffer =
        Buffer::create(mDeviceWrapper,
                       bufferSize,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vks::debug::setDeviceMemoryName(mDeviceWrapper->logicalDevice, stagingBuffer->getMemoryHandle(), "VulkanResources-Image::setContentFromBitmap-stagingBuffer");

    // Copy bitmap pixels to the buffer memory
    void *bitmapData = nullptr;
    assert(AndroidBitmap_lockPixels(env, bitmap, &bitmapData) == ANDROID_BITMAP_RESULT_SUCCESS);
    const bool success = stagingBuffer->copyFrom(bitmapData);
    AndroidBitmap_unlockPixels(env, bitmap);
    assert(success);

    // Copy buffer to image
    VulkanCommandBuffer copyCommand(mDeviceWrapper->logicalDevice, mDeviceWrapper->commandPool);
    assert(mDeviceWrapper->beginSingleTimeCommand(copyCommand.pHandle()));

    // Set layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to prepare for buffer-image copy
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel            = 0;
    subresourceRange.levelCount              = mImageInfo.mipLevels;
    subresourceRange.layerCount              = mImageInfo.arrayLayers;
    setImageLayout(copyCommand.handle(), mImage.handle(),
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    // TODO: Copy mipmaps
    const VkBufferImageCopy bufferImageCopy = {
        .bufferOffset      = 0,
        .bufferRowLength   = info.stride / 4,
        .bufferImageHeight = mImageInfo.extent.height,
        .imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, mImageInfo.arrayLayers},
        .imageOffset       = {0, 0, 0},
        .imageExtent       = mImageInfo.extent,
    };
    vkCmdCopyBufferToImage(copyCommand.handle(),
                           stagingBuffer->getBufferHandle(),
                           mImage.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &bufferImageCopy);

    if (mImageInfo.layout != VK_IMAGE_LAYOUT_UNDEFINED && mImageInfo.layout != VK_IMAGE_LAYOUT_PREINITIALIZED)
    {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel            = 0;
        subresourceRange.levelCount              = mImageInfo.mipLevels;
        subresourceRange.layerCount              = mImageInfo.arrayLayers;
        setImageLayout(copyCommand.handle(), mImage.handle(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mImageInfo.layout, subresourceRange);
    }

    mDeviceWrapper->endAndSubmitSingleTimeCommand(copyCommand.handle(), mVkQueue, false);
    return true;
}

bool Image::createImageFromAHardwareBuffer(AHardwareBuffer *buffer)
{
    // Acquire the AHardwareBuffer and get the descriptor
    AHardwareBuffer_acquire(buffer);
    AHardwareBuffer_Desc ahwbDesc{};
    AHardwareBuffer_describe(buffer, &ahwbDesc);
    mBuffer           = buffer;
    mImageInfo.extent = {ahwbDesc.width, ahwbDesc.height, 1};

    // Get AHardwareBuffer properties
    VkAndroidHardwareBufferFormatPropertiesANDROID formatInfo = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID,
        .pNext = nullptr,
    };
    VkAndroidHardwareBufferPropertiesANDROID properties = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID,
        .pNext = &formatInfo,
    };
    CALL_VK(vkGetAndroidHardwareBufferPropertiesANDROID(
        mDeviceWrapper->logicalDevice, mBuffer, &properties));

    // Create an image to bind to our AHardwareBuffer
    VkExternalMemoryImageCreateInfo externalCreateInfo{
        .sType       = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .pNext       = nullptr,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID,
    };
    VkImageCreateInfo createInfo{
        .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext                 = &externalCreateInfo,
        .flags                 = 0u,
        .imageType             = VK_IMAGE_TYPE_2D,
        .format                = VK_FORMAT_R8G8B8A8_UNORM,
        .extent                = {ahwbDesc.width, ahwbDesc.height, 1u},
        .mipLevels             = 1u,
        .arrayLayers           = 1u,
        .samples               = VK_SAMPLE_COUNT_1_BIT,
        .tiling                = VK_IMAGE_TILING_OPTIMAL,
        .usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    CALL_VK(vkCreateImage(mDeviceWrapper->logicalDevice, &createInfo, nullptr, mImage.pHandle()));

    // Allocate device memory
    uint32_t                                 memoryTypeIndex = mDeviceWrapper->getMemoryType(properties.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkImportAndroidHardwareBufferInfoANDROID androidHardwareBufferInfo{
        .sType  = VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID,
        .pNext  = nullptr,
        .buffer = mBuffer,
    };
    VkMemoryDedicatedAllocateInfo memoryAllocateInfo{
        .sType  = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
        .pNext  = &androidHardwareBufferInfo,
        .image  = mImage.handle(),
        .buffer = VK_NULL_HANDLE,
    };
    VkMemoryAllocateInfo allocateInfo{
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = &memoryAllocateInfo,
        .allocationSize  = properties.allocationSize,
        .memoryTypeIndex = memoryTypeIndex,
    };
    CALL_VK(
        vkAllocateMemory(mDeviceWrapper->logicalDevice, &allocateInfo, nullptr, mMemory.pHandle()));

    // Bind image to the device memory
    CALL_VK(vkBindImageMemory(mDeviceWrapper->logicalDevice, mImage.handle(), mMemory.handle(), 0));
    return true;
}

bool Image::createSampler()
{
    VkSamplerCreateInfo samplerCreateInfo{};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.pNext = isYUVFormat() ? &mSamplerYcbcrConversionInfo : nullptr,
    // 如果用于LUT图，这里采样需要设置为linear的，这样可以做差值保持色彩精度
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter     = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    // Use clamp to edge for BLUR filter
    samplerCreateInfo.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.minLod        = 0.0f;
    samplerCreateInfo.maxLod        = (float) mImageInfo.mipLevels;
    samplerCreateInfo.maxAnisotropy = 1;
    samplerCreateInfo.compareEnable = VK_TRUE;
    samplerCreateInfo.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    // 不归一化坐标:默认false，即归一化坐标。
    // samplerCreateInfo.unnormalizedCoordinates = VK_TRUE;
    CALL_VK(vkCreateSampler(
        mDeviceWrapper->logicalDevice, &samplerCreateInfo, nullptr, mSampler.pHandle()));
    return true;
}

bool Image::createImageView()
{
    const auto getImageViewType = [=](VkImageType imageType) -> VkImageViewType {
        switch (imageType)
        {
            case VK_IMAGE_TYPE_2D:
                if (mImageInfo.arrayLayers > 1)
                {
                    return VK_IMAGE_VIEW_TYPE_CUBE;
                }
                else
                {
                    return VK_IMAGE_VIEW_TYPE_2D;
                }
            case VK_IMAGE_TYPE_3D:
                return VK_IMAGE_VIEW_TYPE_3D;
        }
    };
    const VkImageViewCreateInfo viewCreateInfo{
        .sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext                       = isYUVFormat() ? &mSamplerYcbcrConversionInfo : nullptr,
        .flags                       = 0,
        .image                       = mImage.handle(),
        .viewType                    = getImageViewType(mImageInfo.imageType),
        .format                      = mImageInfo.format,
        .subresourceRange            = {VK_IMAGE_ASPECT_COLOR_BIT, 0, mImageInfo.mipLevels, 0, mImageInfo.arrayLayers},
        .subresourceRange.layerCount = mImageInfo.arrayLayers,
        .subresourceRange.levelCount = mImageInfo.mipLevels,
    };

    CALL_VK(vkCreateImageView(
        mDeviceWrapper->logicalDevice, &viewCreateInfo, nullptr, mImageView.pHandle()));
    return true;
}

bool Image::createSamplerYcbcrConversionInfo()
{
    // Create conversion object that describes how to have the implementation do the {YCbCr} conversion
    VkSamplerYcbcrConversionCreateInfo samplerYcbcrConversionCreateInfo = {VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO};

    // Which 3x3 YUV to RGB matrix is used?
    // 601 is generally used for SD content.
    // 709 for HD content.
    // 2020 for UHD content.
    // Can also use IDENTITY which lets you sample the raw YUV and
    // do the conversion in shader code.
    // At least you don't have to hit the texture unit 3 times.
    samplerYcbcrConversionCreateInfo.ycbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;

    // TV (NARROW) or PC (FULL) range for YUV?
    // Usually, JPEG uses full range and broadcast content is narrow.
    // If using narrow, the YUV components need to be
    // rescaled before it can be converted.
    samplerYcbcrConversionCreateInfo.ycbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_FULL;

    // Deal with order of components.
    samplerYcbcrConversionCreateInfo.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    // With NEAREST, chroma is duplicated to a 2x2 block for YUV420p.
    // In fancy video players, you might even get bicubic/sinc
    // interpolation filters for chroma because why not ...
    samplerYcbcrConversionCreateInfo.chromaFilter = VK_FILTER_LINEAR;

    // COSITED or MIDPOINT
    samplerYcbcrConversionCreateInfo.xChromaOffset = VK_CHROMA_LOCATION_MIDPOINT;
    samplerYcbcrConversionCreateInfo.yChromaOffset = VK_CHROMA_LOCATION_MIDPOINT;

    samplerYcbcrConversionCreateInfo.forceExplicitReconstruction = VK_FALSE;

    // For YUV420p.
    samplerYcbcrConversionCreateInfo.format = mImageInfo.format;

    CALL_VK(vkCreateSamplerYcbcrConversion(mDeviceWrapper->logicalDevice, &samplerYcbcrConversionCreateInfo, nullptr, &mSamplerYcbcrConversion));

    mSamplerYcbcrConversionInfo = {
        .sType      = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO,
        .conversion = mSamplerYcbcrConversion,
    };

    return true;
}

bool Image::isYUVFormat()
{
    switch (mImageInfo.format)
    {
        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
            return true;
    }
    return false;
}

// Create an image memory barrier for changing the layout of
// an image and put it into an active command buffer
// See chapter 11.4 "Image Layout" for details

void Image::setImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout,
                           VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange,
                           VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
    // Create an image barrier object
    VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();
    imageMemoryBarrier.oldLayout            = oldImageLayout;
    imageMemoryBarrier.newLayout            = newImageLayout;
    imageMemoryBarrier.image                = image;
    imageMemoryBarrier.subresourceRange     = subresourceRange;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldImageLayout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            imageMemoryBarrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newImageLayout)
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image will be used as a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image will be used as a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            imageMemoryBarrier.dstAccessMask =
                imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (imageMemoryBarrier.srcAccessMask == 0)
            {
                imageMemoryBarrier.srcAccessMask =
                    VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
    }

    // Put barrier inside setup command buffer
    vkCmdPipelineBarrier(
        cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

// Fixed sub resource on first mip level and layer
void Image::setImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask,
                           VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                           VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask              = aspectMask;
    subresourceRange.baseMipLevel            = 0;
    subresourceRange.levelCount              = 1;
    subresourceRange.layerCount              = 1;
    setImageLayout(cmdbuffer,
                   image,
                   oldImageLayout,
                   newImageLayout,
                   subresourceRange,
                   srcStageMask,
                   dstStageMask);
}

Image::Image(const std::shared_ptr<vks::VulkanDeviceWrapper> context, VkQueue queue, const ImageBasicInfo &imageInfo) :
    mDeviceWrapper(context), mVkQueue(queue), mImage(context->logicalDevice), mMemory(context->logicalDevice), mSampler(context->logicalDevice), mImageView(context->logicalDevice), mImageInfo(imageInfo)
{}

}        // namespace gain
