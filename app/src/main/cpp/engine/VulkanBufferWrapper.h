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

#ifndef GAINVULKANSAMPLE_VULKANBUFFERWRAPPER_H
#define GAINVULKANSAMPLE_VULKANBUFFERWRAPPER_H

#include <gli/gli.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "../util/VulkanRAIIUtil.h"
#include "VulkanDeviceWrapper.hpp"

namespace vks {
    class Buffer {
    public:
        // Create a buffer and allocate the memory.
        static std::unique_ptr <Buffer> create(
                const std::shared_ptr <vks::VulkanDeviceWrapper> deviceWrapper, uint32_t size,
                VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

        // Prefer Buffer::create
        Buffer(const std::shared_ptr <vks::VulkanDeviceWrapper> deviceWrapper, uint32_t size);

        VkBuffer getBufferHandle() const {
            return mBuffer.handle();
        }

        VkDeviceMemory getMemoryHandle() const {
            return mMemory.handle();
        }

        VkDescriptorBufferInfo getDescriptor() const {
            return {mBuffer.handle(), 0, mSize};
        }

        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void unmap();

        void copyFrom(const void* data, VkDeviceSize size);

        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    private:
        bool initialize(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

        const std::shared_ptr <vks::VulkanDeviceWrapper> mContext;
        VkQueue mVkQueue;
        uint32_t mSize;

        // Managed handles
        vks::VulkanBuffer mBuffer;
        vks::VulkanDeviceMemory mMemory;
        void* mapped = nullptr;
        /** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
        VkBufferUsageFlags usageFlags;
        /** @brief Memory property flags to be filled by external source at buffer creation (to query at some later point) */
        VkMemoryPropertyFlags memoryPropertyFlags;
    };
}


#endif //GAINVULKANSAMPLE_VULKANBUFFERWRAPPER_H
