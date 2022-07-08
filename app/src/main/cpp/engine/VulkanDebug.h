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
#pragma once

#include "../util/LogUtil.h"
#include <string>
#include <vulkan_wrapper.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>

namespace vks
{
namespace debug
{
extern bool debugable;

// Load debug function pointers and set debug callback
void setupDebugging(VkInstance instance);

// Clear debug callback
void freeDebugCallback(VkInstance instance);

// Sets the debug name of an object
// All Objects in Vulkan are represented by their 64-bit handles which are passed into this function
// along with the object type
void setObjectName(VkDevice device, uint64_t object, VkObjectType objectType,
                   const char *name);

// Set the tag for an object
void setObjectTag(VkDevice device, uint64_t object, VkObjectType objectType,
                  uint64_t name, size_t tagSize, const void *tag);

// Start a new debug marker region
void beginRegion(VkCommandBuffer cmdbuffer, const char *pMarkerName, glm::vec4 color);

// Insert a new debug marker into the command buffer
void insert(VkCommandBuffer cmdbuffer, std::string markerName, glm::vec4 color);

// End the current debug marker region
void endRegion(VkCommandBuffer cmdBuffer);

// Object specific naming functions
void setCommandBufferName(VkDevice device, VkCommandBuffer cmdBuffer, const char *name);

void setQueueName(VkDevice device, VkQueue queue, const char *name);

void setImageName(VkDevice device, VkImage image, const char *name);

void setSamplerName(VkDevice device, VkSampler sampler, const char *name);

void setBufferName(VkDevice device, VkBuffer buffer, const char *name);

void setDeviceMemoryName(VkDevice device, VkDeviceMemory memory, const char *name);

void setShaderModuleName(VkDevice device, VkShaderModule shaderModule, const char *name);

void setPipelineName(VkDevice device, VkPipeline pipeline, const char *name);

void setPipelineLayoutName(VkDevice device, VkPipelineLayout pipelineLayout, const char *name);

void setRenderPassName(VkDevice device, VkRenderPass renderPass, const char *name);

void setFramebufferName(VkDevice device, VkFramebuffer framebuffer, const char *name);

void setDescriptorSetLayoutName(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char *name);

void setDescriptorSetName(VkDevice device, VkDescriptorSet descriptorSet, const char *name);

void setSemaphoreName(VkDevice device, VkSemaphore semaphore, const char *name);

void setFenceName(VkDevice device, VkFence fence, const char *name);

void setEventName(VkDevice device, VkEvent _event, const char *name);
}        // namespace debug
}        // namespace vks
