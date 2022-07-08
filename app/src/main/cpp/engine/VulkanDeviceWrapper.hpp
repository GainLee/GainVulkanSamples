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

#include "VulkanDebug.h"
#include <LogUtil.h>
#include <algorithm>
#include <assert.h>
#include <cstring>
#include <exception>
#include <vector>
#include <vulkan_wrapper.h>

namespace vks
{
struct VulkanDeviceWrapper
{
    VkPhysicalDevice                     physicalDevice;
    VkDevice                             logicalDevice;
    VkPhysicalDeviceProperties           properties;
    VkPhysicalDeviceFeatures             features;
    VkPhysicalDeviceFeatures             enabledFeatures;
    VkPhysicalDeviceMemoryProperties     memoryProperties;
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    VkCommandPool                        commandPool   = VK_NULL_HANDLE;
    uint32_t                             workGroupSize = 0;

    struct
    {
        uint32_t graphics;
        uint32_t compute;
    } queueFamilyIndices;

    operator VkDevice()
    {
        return logicalDevice;
    };

    /**
	 * Default constructor
	 *
	 * @param physicalDevice Physical device that is to be used
	 */
    VulkanDeviceWrapper(VkPhysicalDevice physicalDevice)
    {
        assert(physicalDevice);
        this->physicalDevice = physicalDevice;

        // Store Properties features, limits and properties of the physical device for later use
        // Device properties also contain limits and sparse properties
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        // Features should be checked by the examples before using them
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);
        // Memory properties are used regularly for creating all kinds of buffers
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
        // Queue family properties, used for setting up requested queues upon device creation
        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        assert(queueFamilyCount > 0);
        queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());
    }

    /**
	 * Default destructor
	 *
	 * @note Frees the logical device
	 */
    ~VulkanDeviceWrapper()
    {
        if (commandPool)
        {
            vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
        }
        if (logicalDevice)
        {
            vkDestroyDevice(logicalDevice, nullptr);
        }
    }

    // Choose the work group size of the compute shader.
    // In this sample app, we are using a square execution dimension.
    uint32_t chooseWorkGroupSize(const VkPhysicalDeviceLimits &limits)
    {
        // Use 64 as the baseline.
        uint32_t size = 64;

        // Make sure the size does not exceed the limit along the X and Y axis.
        size = std::min<uint32_t>(size, limits.maxComputeWorkGroupSize[0]);
        size = std::min<uint32_t>(size, limits.maxComputeWorkGroupSize[1]);

        // Make sure the total number of invocations does not exceed the limit.
        size = std::min<uint32_t>(
            size, static_cast<uint32_t>(sqrt(limits.maxComputeWorkGroupInvocations)));

        // We prefer the workgroup size to be a multiple of 4.
        size &= ~(3u);

        LOGCATI("maxComputeWorkGroupInvocations: %d, maxComputeWorkGroupSize: (%d, %d)",
                limits.maxComputeWorkGroupInvocations, limits.maxComputeWorkGroupSize[0],
                limits.maxComputeWorkGroupSize[1]);
        LOGCATI("Choose workgroup size: (%d, %d)", size, size);
        return size;
    }

    /**
	 * Get the index of a memory type that has all the requested property bits set
	 *
	 * @param typeBits Bitmask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
	 * @param properties Bitmask of properties for the memory type to request
	 * @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
	 *
	 * @return Index of the requested memory type
	 *
	 * @throw Throws an exception if memTypeFound is null and no memory type could be found that supports the requested properties
	 */
    uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr) const
    {
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    if (memTypeFound)
                    {
                        *memTypeFound = true;
                    }
                    return i;
                }
            }
            typeBits >>= 1;
        }

        if (memTypeFound)
        {
            *memTypeFound = false;
            return 0;
        }
        else
        {
            throw std::runtime_error("Could not find a matching memory type");
        }
    }

    /**
	 * Get the index of a queue family that supports the requested queue flags
	 *
	 * @param queueFlags Queue flags to find a queue family index for
	 *
	 * @return Index of the queue family index that matches the flags
	 *
	 * @throw Throws an exception if no queue family index could be found that supports the requested flags
	 */
    uint32_t getQueueFamilyIndex(VkQueueFlagBits queueFlags)
    {
        // Dedicated queue for compute
        // Try to find a queue family index that supports compute but not graphics
        if (queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
            {
                if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    return i;
                    break;
                }
            }
        }

        // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
        {
            if (queueFamilyProperties[i].queueFlags & queueFlags)
            {
                return i;
                break;
            }
        }

        throw std::runtime_error("Could not find a matching queue family index");
    }

    void getDepthFormat(VkFormat &depthFormat)
    {
        // Get depth format
        //  Since all depth formats may be optional, we need to find a suitable depth format to use
        //  Start with the highest precision packed format
        std::vector<VkFormat> depthFormats = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM};

        for (auto &format : depthFormats)
        {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
            // Format must support depth stencil attachment for optimal tiling
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                depthFormat = format;
                break;
            }
        }
    }

    /**
	 * Create the logical device based on the assigned physical device, also gets default queue family indices
	 *
	 * @param enabledFeatures Can be used to enable certain features upon device creation
	 * @param requestedQueueTypes Bit flags specifying the queue types to be requested from the device
	 *
	 * @return VkResult of the device creation call
	 */
    VkResult createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char *> enabledExtensions, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)
    {
        // Desired queues need to be requested upon logical device creation
        // Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
        // requests different queue types

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

        // Get queue family indices for the requested queue family types
        // Note that the indices may overlap depending on the implementation

        // 挑选支持指定Flag Bit的设备
        // 需要了解的是，不同的硬件可能支持不同的 Flag Bit 组合，但 Vulkan 规范指出，任何 Vulkan 设备至少有一种
        // Graphics 和 Compute 的组合，同时，无论是 Graphics 还是 Compute，都隐含的支持 Transfer 命令，也就是说，
        // 任何 Vulkan 硬件，都至少有一个可以执行 Graphics、Compute 和 Transfer 命令的 Queue 族。尽管如此，
        // Vulkan 规范并不要求实现必须要上报 Transfer，这是实现的可选项，所以通过
        // vkGetPhysicalDeviceQueueFamilyProperties 获取到的 Graphics | Compute 组合可能不一定包含
        // Transfer Flag Bit，但实际上是支持 Transfer 命令的。
        const float defaultQueuePriority(0.0f);

        // Graphics queue
        if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
        {
            queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
            queueInfo.queueCount       = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInfos.push_back(queueInfo);
        }
        else
        {
            queueFamilyIndices.graphics = VK_NULL_HANDLE;
        }

        // Dedicated compute queue
        if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
        {
            queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
            if (queueFamilyIndices.compute != queueFamilyIndices.graphics)
            {
                // If compute family index differs, we need an additional queue create info for the compute queue
                VkDeviceQueueCreateInfo queueInfo{};
                queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
                queueInfo.queueCount       = 1;
                queueInfo.pQueuePriorities = &defaultQueuePriority;
                queueCreateInfos.push_back(queueInfo);
            }
        }
        else
        {
            // Else we use the same queue
            queueFamilyIndices.compute = queueFamilyIndices.graphics;
        }

        // Create the logical device representation
        std::vector<const char *> deviceExtensions(enabledExtensions);
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        VkDeviceCreateInfo deviceCreateInfo   = {};
        deviceCreateInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        ;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures  = &enabledFeatures;

        if (deviceExtensions.size() > 0)
        {
            deviceCreateInfo.enabledExtensionCount   = (uint32_t) deviceExtensions.size();
            deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        }

        // Enable extensions' features
        auto isEnabled = [&](const char *extension) {
            return std::find_if(enabledExtensions.begin(), enabledExtensions.end(), [extension](const char *enabled_extension) { return strcmp(extension, enabled_extension) == 0; }) != enabledExtensions.end();
        };
        if (isEnabled(VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME))
        {
            VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR samplerYcbcrConversionFeature = {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES,
                nullptr};
            VkPhysicalDeviceFeatures2KHR enabledFeatures =
                {
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR,        // sType
                    &samplerYcbcrConversionFeature,                          // pNext
                };
            samplerYcbcrConversionFeature.samplerYcbcrConversion = VK_TRUE;
            deviceCreateInfo.pNext                               = &enabledFeatures;
            deviceCreateInfo.pEnabledFeatures                    = nullptr;
        }

        VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);

        if (result == VK_SUCCESS)
        {
            commandPool = createCommandPool(queueFamilyIndices.graphics);
        }

        this->enabledFeatures = enabledFeatures;

        workGroupSize = chooseWorkGroupSize(properties.limits);

        return result;
    }

    /**
	 * Create a command pool for allocation command buffers from
	 *
	 * @param queueFamilyIndex Family index of the queue to create the command pool for
	 * @param createFlags (Optional) Command pool creation flags (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
	 *
	 * @note Command buffers allocated from the created pool can only be submitted to a queue with the same family index
	 *
	 * @return A handle to the created command buffer
	 */
    VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
    {
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex        = queueFamilyIndex;
        cmdPoolInfo.flags                   = createFlags;
        VkCommandPool cmdPool;
        CALL_VK(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool));
        return cmdPool;
    }

    /**
	 * Allocate a command buffer from the command pool
	 *
	 * @param level Level of the new command buffer (primary or secondary)
	 * @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
	 *
	 * @return A handle to the allocated command buffer
	 */
    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin = false)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool        = commandPool;
        cmdBufAllocateInfo.level              = level;
        cmdBufAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer cmdBuffer;
        CALL_VK(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));

        // If requested, also start recording for the new command buffer
        if (begin)
        {
            VkCommandBufferBeginInfo commandBufferBI{};
            commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            CALL_VK(vkBeginCommandBuffer(cmdBuffer, &commandBufferBI));
        }

        return cmdBuffer;
    }

    void beginCommandBuffer(VkCommandBuffer commandBuffer)
    {
        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CALL_VK(vkBeginCommandBuffer(commandBuffer, &commandBufferBI));
    }

    // Create a command buffer with VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, and begin command
    // buffer recording.
    bool beginSingleTimeCommand(VkCommandBuffer *commandBuffer) const
    {
        if (commandBuffer == nullptr)
            return false;

        // Allocate a command buffer
        const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = commandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        CALL_VK(vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, commandBuffer));

        // Begin command buffer recording
        const VkCommandBufferBeginInfo beginInfo = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };
        CALL_VK(vkBeginCommandBuffer(*commandBuffer, &beginInfo));
        return true;
    }

    // End the command buffer recording, submit it to the queue, and wait until it is finished.
    void endAndSubmitSingleTimeCommand(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true) const
    {
        vks::debug::setCommandBufferName(logicalDevice, commandBuffer, "SingleTimeCommand");

        CALL_VK(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer;

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence;
        CALL_VK(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));

        // Submit to the queue
        CALL_VK(vkQueueSubmit(queue, 1, &submitInfo, fence));
        // Wait for the fence to signal that command buffer has finished executing
        CALL_VK(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, 100000000000));

        vkDestroyFence(logicalDevice, fence, nullptr);

        if (free)
        {
            vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
        }
    }
};
}        // namespace vks
