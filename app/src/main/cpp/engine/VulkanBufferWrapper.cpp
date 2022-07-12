/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Gain
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

#include "VulkanBufferWrapper.h"

namespace vks {
    std::unique_ptr <Buffer>
    Buffer::create(const std::shared_ptr <vks::VulkanDeviceWrapper> context,
                   uint32_t size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties) {
        auto buffer = std::make_unique<Buffer>(context, size);
        const bool success = buffer->initialize(usage, properties);
        return success ? std::move(buffer) : nullptr;
    }

    Buffer::Buffer(const std::shared_ptr <vks::VulkanDeviceWrapper> context, uint32_t size) :
            mContext(context), mSize(size), mBuffer(context->logicalDevice),
            mMemory(context->logicalDevice) {}

    bool Buffer::initialize(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        // Create buffer
        const VkBufferCreateInfo bufferCreateInfo = {
                .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size        = mSize,
                .usage       = usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        CALL_VK(vkCreateBuffer(mContext->logicalDevice, &bufferCreateInfo, nullptr,
                               mBuffer.pHandle()));

        // Allocate memory for the buffer
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(mContext->logicalDevice, mBuffer.handle(),
                                      &memoryRequirements);
        const auto memoryTypeIndex =
                mContext->getMemoryType(memoryRequirements.memoryTypeBits, properties);
        const VkMemoryAllocateInfo allocateInfo = {
                .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext           = nullptr,
                .allocationSize  = memoryRequirements.size,
                .memoryTypeIndex = memoryTypeIndex,
        };
        CALL_VK(vkAllocateMemory(mContext->logicalDevice, &allocateInfo, nullptr,
                                 mMemory.pHandle()));

        vkBindBufferMemory(mContext->logicalDevice, mBuffer.handle(), mMemory.handle(), 0);

        vks::debug::setDeviceMemoryName(mContext->logicalDevice, mMemory.handle(),
                                        "VulkanResources-Buffer::initialize-mMemory");

        return true;
    }

    /**
	* Copies the specified data to the mapped buffer
	*
	* @param data Pointer to the data to copy
	* @param size Size of the data to copy in machine units
	*
	*/
    void Buffer::copyFrom(const void* data, VkDeviceSize size)
    {
        assert(mapped);
        memcpy(mapped, data, size);
    }

    /**
	* Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
	*
	* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the buffer mapping call
	*/
    VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset)
    {
        return vkMapMemory(mContext->logicalDevice, mMemory.handle(), offset, size, 0, &mapped);
    }

    /**
    * Unmap a mapped memory range
    *
    * @note Does not return a result as vkUnmapMemory can't fail
    */
    void Buffer::unmap()
    {
        if (mapped)
        {
            vkUnmapMemory(mContext->logicalDevice, mMemory.handle());
            mapped = nullptr;
        }
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
    VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = mMemory.handle();
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkFlushMappedMemoryRanges(mContext->logicalDevice, 1, &mappedRange);
    }

    /**
	* Invalidate a memory range of the buffer to make it visible to the host
	*
	* @note Only required for non-coherent memory
	*
	* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the invalidate call
	*/
    VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
    {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = mMemory.handle();
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkInvalidateMappedMemoryRanges(mContext->logicalDevice, 1, &mappedRange);
    }
}