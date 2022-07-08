/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 by Gain
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
#include "VulkanDebug.h"
#include <iostream>
#include <sstream>

namespace vks
{
namespace debug
{
bool debugable = true;

PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
PFN_vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectNameEXT;
PFN_vkSetDebugUtilsObjectTagEXT     vkSetDebugUtilsObjectTagEXT;
PFN_vkCmdBeginDebugUtilsLabelEXT    vkCmdBeginDebugUtilsLabelEXT;
PFN_vkCmdInsertDebugUtilsLabelEXT   vkCmdInsertDebugUtilsLabelEXT;
PFN_vkCmdEndDebugUtilsLabelEXT      vkCmdEndDebugUtilsLabelEXT;

VkDebugUtilsMessengerEXT debugUtilsMessenger;

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    // Select prefix depending on flags passed to the callback
    std::string prefix("");

    if (messageSeverity &
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        prefix = "VERBOSE: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        prefix = "INFO: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        prefix = "WARNING: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        prefix = "ERROR: ";
    }

    // Display message to default output (console/logcat)
    std::stringstream debugMessage;
    debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "]["
                 << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LOGCATE("%s", debugMessage.str().c_str());
    }
    else
    {
        LOGCATD("%s", debugMessage.str().c_str());
    }

    // The return value of this callback controls whether the Vulkan call that caused the validation
    // message will be aborted or not We return VK_FALSE as we DON'T want Vulkan calls that cause a
    // validation message to abort If you instead want to have calls abort, pass in VK_TRUE and the
    // function will return VK_ERROR_VALIDATION_FAILED_EXT
    return VK_FALSE;
}

void setupDebugging(VkInstance instance)
{
    vkCreateDebugUtilsMessengerEXT  = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    vkSetDebugUtilsObjectNameEXT  = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
    vkSetDebugUtilsObjectTagEXT   = reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectTagEXT"));
    vkCmdBeginDebugUtilsLabelEXT  = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
    vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
    vkCmdEndDebugUtilsLabelEXT    = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
    debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerCI.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugUtilsMessengerCI.pfnUserCallback = debugUtilsMessengerCallback;
    VkResult result                       = vkCreateDebugUtilsMessengerEXT(
        instance, &debugUtilsMessengerCI, nullptr, &debugUtilsMessenger);
    assert(result == VK_SUCCESS);
}

void freeDebugCallback(VkInstance instance)
{
    if (debugUtilsMessenger != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
    }
}

void setObjectName(VkDevice device, uint64_t object, VkObjectType objectType,
                   const char *name)
{
    // Check for valid function pointer (may not be present if not running in a debugging
    // application)
    if (vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType                         = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType                    = objectType;
        nameInfo.objectHandle                  = object;
        nameInfo.pObjectName                   = name;
        vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
    }
}

void setObjectTag(VkDevice device, uint64_t object, VkObjectType objectType,
                  uint64_t name, size_t tagSize, const void *tag)
{
    // Check for valid function pointer (may not be present if not running in a debugging
    // application)
    if (vkSetDebugUtilsObjectTagEXT)
    {
        VkDebugUtilsObjectTagInfoEXT tagInfo = {};
        tagInfo.sType                        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
        tagInfo.objectType                   = objectType;
        tagInfo.objectHandle                 = object;
        tagInfo.tagName                      = name;
        tagInfo.tagSize                      = tagSize;
        tagInfo.pTag                         = tag;
        vkSetDebugUtilsObjectTagEXT(device, &tagInfo);
    }
}

void beginRegion(VkCommandBuffer cmdbuffer, const char *pMarkerName, glm::vec4 color)
{
    // Check for valid function pointer (may not be present if not running in a debugging
    // application)
    if (vkCmdBeginDebugUtilsLabelEXT)
    {
        VkDebugUtilsLabelEXT markerInfo = {};
        markerInfo.sType                = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
        markerInfo.pLabelName = pMarkerName;
        vkCmdBeginDebugUtilsLabelEXT(cmdbuffer, &markerInfo);
    }
}

void insert(VkCommandBuffer cmdbuffer, std::string markerName, glm::vec4 color)
{
    // Check for valid function pointer (may not be present if not running in a debugging
    // application)
    if (vkCmdInsertDebugUtilsLabelEXT)
    {
        VkDebugUtilsLabelEXT markerInfo = {};
        markerInfo.sType                = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
        markerInfo.pLabelName = markerName.c_str();
        vkCmdInsertDebugUtilsLabelEXT(cmdbuffer, &markerInfo);
    }
}

void endRegion(VkCommandBuffer cmdBuffer)
{
    // Check for valid function (may not be present if not running in a debugging application)
    if (vkCmdEndDebugUtilsLabelEXT)
    {
        vkCmdEndDebugUtilsLabelEXT(cmdBuffer);
    }
}

void setCommandBufferName(VkDevice device, VkCommandBuffer cmdBuffer, const char *name)
{
    setObjectName(
        device, (uint64_t) cmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER,
        name);
}

void setQueueName(VkDevice device, VkQueue queue, const char *name)
{
    setObjectName(device, (uint64_t) queue, VK_OBJECT_TYPE_QUEUE, name);
}

void setImageName(VkDevice device, VkImage image, const char *name)
{
    setObjectName(device, (uint64_t) image, VK_OBJECT_TYPE_IMAGE, name);
}

void setSamplerName(VkDevice device, VkSampler sampler, const char *name)
{
    setObjectName(device, (uint64_t) sampler, VK_OBJECT_TYPE_SAMPLER,
                  name);
}

void setBufferName(VkDevice device, VkBuffer buffer, const char *name)
{
    setObjectName(device, (uint64_t) buffer, VK_OBJECT_TYPE_BUFFER, name);
}

void setDeviceMemoryName(VkDevice device, VkDeviceMemory memory, const char *name)
{
    setObjectName(device, (uint64_t) memory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                  name);
}

void setShaderModuleName(VkDevice device, VkShaderModule shaderModule, const char *name)
{
    setObjectName(
        device, (uint64_t) shaderModule, VK_OBJECT_TYPE_SHADER_MODULE,
        name);
}

void setPipelineName(VkDevice device, VkPipeline pipeline, const char *name)
{
    setObjectName(device, (uint64_t) pipeline, VK_OBJECT_TYPE_PIPELINE,
                  name);
}

void setPipelineLayoutName(VkDevice device, VkPipelineLayout pipelineLayout, const char *name)
{
    setObjectName(
        device, (uint64_t) pipelineLayout,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT, name);
}

void setRenderPassName(VkDevice device, VkRenderPass renderPass, const char *name)
{
    setObjectName(device, (uint64_t) renderPass,
                  VK_OBJECT_TYPE_RENDER_PASS, name);
}

void setFramebufferName(VkDevice device, VkFramebuffer framebuffer, const char *name)
{
    setObjectName(device, (uint64_t) framebuffer,
                  VK_OBJECT_TYPE_FRAMEBUFFER, name);
}

void setDescriptorSetLayoutName(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
                                const char *name)
{
    setObjectName(device,
                  (uint64_t) descriptorSetLayout,
                  VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                  name);
}

void setDescriptorSetName(VkDevice device, VkDescriptorSet descriptorSet, const char *name)
{
    setObjectName(
        device, (uint64_t) descriptorSet,
        VK_OBJECT_TYPE_DESCRIPTOR_SET, name);
}

void setSemaphoreName(VkDevice device, VkSemaphore semaphore, const char *name)
{
    setObjectName(device, (uint64_t) semaphore, VK_OBJECT_TYPE_SEMAPHORE,
                  name);
}

void setFenceName(VkDevice device, VkFence fence, const char *name)
{
    setObjectName(device, (uint64_t) fence, VK_OBJECT_TYPE_FENCE, name);
}

void setEventName(VkDevice device, VkEvent _event, const char *name)
{
    setObjectName(device, (uint64_t) _event, VK_OBJECT_TYPE_EVENT, name);
}
}        // namespace debug
}        // namespace vks
