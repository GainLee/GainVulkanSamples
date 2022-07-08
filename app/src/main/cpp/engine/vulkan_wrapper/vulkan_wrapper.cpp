/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 by Gain
 * Copyright (C) 2022 by Google Inc
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

// This file is generated.
#include "vulkan_wrapper.h"
#include <dlfcn.h>

PFN_vkCreateInstance vkCreateInstance;
PFN_vkDestroyInstance vkDestroyInstance;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
PFN_vkCreateDevice vkCreateDevice;
PFN_vkDestroyDevice vkDestroyDevice;
PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties;
PFN_vkGetDeviceQueue vkGetDeviceQueue;
PFN_vkQueueSubmit vkQueueSubmit;
PFN_vkQueueWaitIdle vkQueueWaitIdle;
PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
PFN_vkAllocateMemory vkAllocateMemory;
PFN_vkFreeMemory vkFreeMemory;
PFN_vkMapMemory vkMapMemory;
PFN_vkUnmapMemory vkUnmapMemory;
PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment;
PFN_vkBindBufferMemory vkBindBufferMemory;
PFN_vkBindImageMemory vkBindImageMemory;
PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties;
PFN_vkQueueBindSparse vkQueueBindSparse;
PFN_vkCreateFence vkCreateFence;
PFN_vkDestroyFence vkDestroyFence;
PFN_vkResetFences vkResetFences;
PFN_vkGetFenceStatus vkGetFenceStatus;
PFN_vkWaitForFences vkWaitForFences;
PFN_vkCreateSemaphore vkCreateSemaphore;
PFN_vkDestroySemaphore vkDestroySemaphore;
PFN_vkCreateEvent vkCreateEvent;
PFN_vkDestroyEvent vkDestroyEvent;
PFN_vkGetEventStatus vkGetEventStatus;
PFN_vkSetEvent vkSetEvent;
PFN_vkResetEvent vkResetEvent;
PFN_vkCreateQueryPool vkCreateQueryPool;
PFN_vkDestroyQueryPool vkDestroyQueryPool;
PFN_vkGetQueryPoolResults vkGetQueryPoolResults;
PFN_vkCreateBuffer vkCreateBuffer;
PFN_vkDestroyBuffer vkDestroyBuffer;
PFN_vkCreateBufferView vkCreateBufferView;
PFN_vkDestroyBufferView vkDestroyBufferView;
PFN_vkCreateImage vkCreateImage;
PFN_vkDestroyImage vkDestroyImage;
PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout;
PFN_vkCreateImageView vkCreateImageView;
PFN_vkDestroyImageView vkDestroyImageView;
PFN_vkCreateShaderModule vkCreateShaderModule;
PFN_vkDestroyShaderModule vkDestroyShaderModule;
PFN_vkCreatePipelineCache vkCreatePipelineCache;
PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
PFN_vkGetPipelineCacheData vkGetPipelineCacheData;
PFN_vkMergePipelineCaches vkMergePipelineCaches;
PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
PFN_vkCreateComputePipelines vkCreateComputePipelines;
PFN_vkDestroyPipeline vkDestroyPipeline;
PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
PFN_vkCreateSampler vkCreateSampler;
PFN_vkDestroySampler vkDestroySampler;
PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
PFN_vkResetDescriptorPool vkResetDescriptorPool;
PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
PFN_vkCreateFramebuffer vkCreateFramebuffer;
PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
PFN_vkCreateRenderPass vkCreateRenderPass;
PFN_vkDestroyRenderPass vkDestroyRenderPass;
PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity;
PFN_vkCreateCommandPool vkCreateCommandPool;
PFN_vkDestroyCommandPool vkDestroyCommandPool;
PFN_vkResetCommandPool vkResetCommandPool;
PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
PFN_vkEndCommandBuffer vkEndCommandBuffer;
PFN_vkResetCommandBuffer vkResetCommandBuffer;
PFN_vkCmdBindPipeline vkCmdBindPipeline;
PFN_vkCmdSetViewport vkCmdSetViewport;
PFN_vkCmdSetScissor vkCmdSetScissor;
PFN_vkCmdSetLineWidth vkCmdSetLineWidth;
PFN_vkCmdSetDepthBias vkCmdSetDepthBias;
PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants;
PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds;
PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask;
PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask;
PFN_vkCmdSetStencilReference vkCmdSetStencilReference;
PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
PFN_vkCmdDraw vkCmdDraw;
PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
PFN_vkCmdDrawIndirect vkCmdDrawIndirect;
PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
PFN_vkCmdDispatch vkCmdDispatch;
PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect;
PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
PFN_vkCmdCopyImage vkCmdCopyImage;
PFN_vkCmdBlitImage vkCmdBlitImage;
PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer;
PFN_vkCmdFillBuffer vkCmdFillBuffer;
PFN_vkCmdClearColorImage vkCmdClearColorImage;
PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage;
PFN_vkCmdClearAttachments vkCmdClearAttachments;
PFN_vkCmdResolveImage vkCmdResolveImage;
PFN_vkCmdSetEvent vkCmdSetEvent;
PFN_vkCmdResetEvent vkCmdResetEvent;
PFN_vkCmdWaitEvents vkCmdWaitEvents;
PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
PFN_vkCmdBeginQuery vkCmdBeginQuery;
PFN_vkCmdEndQuery vkCmdEndQuery;
PFN_vkCmdResetQueryPool vkCmdResetQueryPool;
PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp;
PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults;
PFN_vkCmdPushConstants vkCmdPushConstants;
PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
PFN_vkCmdNextSubpass vkCmdNextSubpass;
PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
PFN_vkCmdExecuteCommands vkCmdExecuteCommands;
PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
PFN_vkQueuePresentKHR vkQueuePresentKHR;
PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR;
PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR;
PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR;
PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR;
PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR;
PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR;
PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR;
PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR;
PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2;
PFN_vkBindImageMemory2KHR vkBindImageMemory2;
PFN_vkCreateSamplerYcbcrConversionKHR vkCreateSamplerYcbcrConversion;
PFN_vkDestroySamplerYcbcrConversionKHR vkDestroySamplerYcbcrConversion;
PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2;

#ifdef VK_USE_PLATFORM_XLIB_KHR
PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR;
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR;
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR;
#endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
PFN_vkGetAndroidHardwareBufferPropertiesANDROID vkGetAndroidHardwareBufferPropertiesANDROID;
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;
#endif

void *libVulkan;

// Dynamically load Vulkan library and base function pointers
bool loadVulkanLibrary()
{

    // Load vulkan library
    libVulkan = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!libVulkan)
    {
        return false;
    }

    // Load base function pointers
    vkEnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(dlsym(libVulkan, "vkEnumerateInstanceExtensionProperties"));
    vkEnumerateInstanceLayerProperties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(dlsym(libVulkan, "vkEnumerateInstanceLayerProperties"));
    vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(dlsym(libVulkan, "vkCreateInstance"));
    vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(libVulkan, "vkGetInstanceProcAddr"));
    vkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(dlsym(libVulkan, "vkGetDeviceProcAddr"));

    return true;
}

// Load instance based Vulkan function pointers
void loadVulkanFunctions(VkInstance instance) {

    // Vulkan supported, set function addresses
    vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(vkGetInstanceProcAddr(instance,
                                                                      "vkDestroyInstance"));
    vkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(vkGetInstanceProcAddr(instance,
                                                                                        "vkEnumeratePhysicalDevices"));
    vkGetPhysicalDeviceFeatures =
            reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(vkGetInstanceProcAddr(instance,
                                                                    "vkGetPhysicalDeviceFeatures"));
    vkGetPhysicalDeviceFormatProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(vkGetInstanceProcAddr(instance,
                                                                            "vkGetPhysicalDeviceFormatProperties"));
    vkGetPhysicalDeviceImageFormatProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceImageFormatProperties>(
            vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties"));
    vkGetPhysicalDeviceProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(vkGetInstanceProcAddr(instance,
                                                                      "vkGetPhysicalDeviceProperties"));
    vkGetPhysicalDeviceQueueFamilyProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
            vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    vkGetPhysicalDeviceMemoryProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(vkGetInstanceProcAddr(instance,
                                                                            "vkGetPhysicalDeviceMemoryProperties"));
    vkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(vkGetInstanceProcAddr(instance, "vkCreateDevice"));
    vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(vkGetInstanceProcAddr(instance, "vkDestroyDevice"));
    vkEnumerateDeviceExtensionProperties =
            reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(vkGetInstanceProcAddr(instance,
                                                                             "vkEnumerateDeviceExtensionProperties"));
    vkEnumerateDeviceLayerProperties =
            reinterpret_cast<PFN_vkEnumerateDeviceLayerProperties>(vkGetInstanceProcAddr(instance,
                                                                         "vkEnumerateDeviceLayerProperties"));
    vkGetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(vkGetInstanceProcAddr(instance, "vkGetDeviceQueue"));
    vkQueueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(vkGetInstanceProcAddr(instance, "vkQueueSubmit"));
    vkQueueWaitIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(vkGetInstanceProcAddr(instance, "vkQueueWaitIdle"));
    vkDeviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(vkGetInstanceProcAddr(instance, "vkDeviceWaitIdle"));
    vkAllocateMemory = reinterpret_cast<PFN_vkAllocateMemory>(vkGetInstanceProcAddr(instance, "vkAllocateMemory"));
    vkFreeMemory = reinterpret_cast<PFN_vkFreeMemory>(vkGetInstanceProcAddr(instance, "vkFreeMemory"));
    vkMapMemory = reinterpret_cast<PFN_vkMapMemory>(vkGetInstanceProcAddr(instance, "vkMapMemory"));
    vkUnmapMemory = reinterpret_cast<PFN_vkUnmapMemory>(vkGetInstanceProcAddr(instance, "vkUnmapMemory"));
    vkFlushMappedMemoryRanges = reinterpret_cast<PFN_vkFlushMappedMemoryRanges>(vkGetInstanceProcAddr(instance,
                                                                                      "vkFlushMappedMemoryRanges"));
    vkInvalidateMappedMemoryRanges =
            reinterpret_cast<PFN_vkInvalidateMappedMemoryRanges>(vkGetInstanceProcAddr(instance,
                                                                       "vkInvalidateMappedMemoryRanges"));
    vkGetDeviceMemoryCommitment =
            reinterpret_cast<PFN_vkGetDeviceMemoryCommitment>(vkGetInstanceProcAddr(instance,
                                                                    "vkGetDeviceMemoryCommitment"));
    vkBindBufferMemory = reinterpret_cast<PFN_vkBindBufferMemory>(vkGetInstanceProcAddr(instance,
                                                                        "vkBindBufferMemory"));
    vkBindImageMemory = reinterpret_cast<PFN_vkBindImageMemory>(vkGetInstanceProcAddr(instance,
                                                                      "vkBindImageMemory"));
    vkGetBufferMemoryRequirements =
            reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(vkGetInstanceProcAddr(instance,
                                                                      "vkGetBufferMemoryRequirements"));
    vkGetImageMemoryRequirements =
            reinterpret_cast<PFN_vkGetImageMemoryRequirements>(vkGetInstanceProcAddr(instance,
                                                                     "vkGetImageMemoryRequirements"));
    vkGetImageSparseMemoryRequirements =
            reinterpret_cast<PFN_vkGetImageSparseMemoryRequirements>(vkGetInstanceProcAddr(instance,
                                                                           "vkGetImageSparseMemoryRequirements"));
    vkGetPhysicalDeviceSparseImageFormatProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceSparseImageFormatProperties>(
            vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSparseImageFormatProperties"));
    vkQueueBindSparse = reinterpret_cast<PFN_vkQueueBindSparse>(vkGetInstanceProcAddr(instance,
                                                                      "vkQueueBindSparse"));
    vkCreateFence = reinterpret_cast<PFN_vkCreateFence>(vkGetInstanceProcAddr(instance, "vkCreateFence"));
    vkDestroyFence = reinterpret_cast<PFN_vkDestroyFence>(vkGetInstanceProcAddr(instance, "vkDestroyFence"));
    vkResetFences = reinterpret_cast<PFN_vkResetFences>(vkGetInstanceProcAddr(instance, "vkResetFences"));
    vkGetFenceStatus = reinterpret_cast<PFN_vkGetFenceStatus>(vkGetInstanceProcAddr(instance, "vkGetFenceStatus"));
    vkWaitForFences = reinterpret_cast<PFN_vkWaitForFences>(vkGetInstanceProcAddr(instance, "vkWaitForFences"));
    vkCreateSemaphore = reinterpret_cast<PFN_vkCreateSemaphore>(vkGetInstanceProcAddr(instance,
                                                                      "vkCreateSemaphore"));
    vkDestroySemaphore = reinterpret_cast<PFN_vkDestroySemaphore>(vkGetInstanceProcAddr(instance,
                                                                        "vkDestroySemaphore"));
    vkCreateEvent = reinterpret_cast<PFN_vkCreateEvent>(vkGetInstanceProcAddr(instance, "vkCreateEvent"));
    vkDestroyEvent = reinterpret_cast<PFN_vkDestroyEvent>(vkGetInstanceProcAddr(instance, "vkDestroyEvent"));
    vkGetEventStatus = reinterpret_cast<PFN_vkGetEventStatus>(vkGetInstanceProcAddr(instance, "vkGetEventStatus"));
    vkSetEvent = reinterpret_cast<PFN_vkSetEvent>(vkGetInstanceProcAddr(instance, "vkSetEvent"));
    vkResetEvent = reinterpret_cast<PFN_vkResetEvent>(vkGetInstanceProcAddr(instance, "vkResetEvent"));
    vkCreateQueryPool = reinterpret_cast<PFN_vkCreateQueryPool>(vkGetInstanceProcAddr(instance,
                                                                      "vkCreateQueryPool"));
    vkDestroyQueryPool = reinterpret_cast<PFN_vkDestroyQueryPool>(vkGetInstanceProcAddr(instance,
                                                                        "vkDestroyQueryPool"));
    vkGetQueryPoolResults = reinterpret_cast<PFN_vkGetQueryPoolResults>(vkGetInstanceProcAddr(instance,
                                                                              "vkGetQueryPoolResults"));
    vkCreateBuffer = reinterpret_cast<PFN_vkCreateBuffer>(vkGetInstanceProcAddr(instance, "vkCreateBuffer"));
    vkDestroyBuffer = reinterpret_cast<PFN_vkDestroyBuffer>(vkGetInstanceProcAddr(instance, "vkDestroyBuffer"));
    vkCreateBufferView = reinterpret_cast<PFN_vkCreateBufferView>(vkGetInstanceProcAddr(instance,
                                                                        "vkCreateBufferView"));
    vkDestroyBufferView = reinterpret_cast<PFN_vkDestroyBufferView>(vkGetInstanceProcAddr(instance,
                                                                          "vkDestroyBufferView"));
    vkCreateImage = reinterpret_cast<PFN_vkCreateImage>(vkGetInstanceProcAddr(instance, "vkCreateImage"));
    vkDestroyImage = reinterpret_cast<PFN_vkDestroyImage>(vkGetInstanceProcAddr(instance, "vkDestroyImage"));
    vkGetImageSubresourceLayout =
            reinterpret_cast<PFN_vkGetImageSubresourceLayout>(vkGetInstanceProcAddr(instance,
                                                                    "vkGetImageSubresourceLayout"));
    vkCreateImageView = reinterpret_cast<PFN_vkCreateImageView>(vkGetInstanceProcAddr(instance,
                                                                      "vkCreateImageView"));
    vkDestroyImageView = reinterpret_cast<PFN_vkDestroyImageView>(vkGetInstanceProcAddr(instance,
                                                                        "vkDestroyImageView"));
    vkCreateShaderModule = reinterpret_cast<PFN_vkCreateShaderModule>(vkGetInstanceProcAddr(instance,
                                                                            "vkCreateShaderModule"));
    vkDestroyShaderModule = reinterpret_cast<PFN_vkDestroyShaderModule>(vkGetInstanceProcAddr(instance,
                                                                              "vkDestroyShaderModule"));
    vkCreatePipelineCache = reinterpret_cast<PFN_vkCreatePipelineCache>(vkGetInstanceProcAddr(instance,
                                                                              "vkCreatePipelineCache"));
    vkDestroyPipelineCache = reinterpret_cast<PFN_vkDestroyPipelineCache>(vkGetInstanceProcAddr(instance,
                                                                                "vkDestroyPipelineCache"));
    vkGetPipelineCacheData = reinterpret_cast<PFN_vkGetPipelineCacheData>(vkGetInstanceProcAddr(instance,
                                                                                "vkGetPipelineCacheData"));
    vkMergePipelineCaches = reinterpret_cast<PFN_vkMergePipelineCaches>(vkGetInstanceProcAddr(instance,
                                                                              "vkMergePipelineCaches"));
    vkCreateGraphicsPipelines = reinterpret_cast<PFN_vkCreateGraphicsPipelines>(vkGetInstanceProcAddr(instance,
                                                                                      "vkCreateGraphicsPipelines"));
    vkCreateComputePipelines = reinterpret_cast<PFN_vkCreateComputePipelines>(vkGetInstanceProcAddr(instance,
                                                                                    "vkCreateComputePipelines"));
    vkDestroyPipeline = reinterpret_cast<PFN_vkDestroyPipeline>(vkGetInstanceProcAddr(instance,
                                                                      "vkDestroyPipeline"));
    vkCreatePipelineLayout = reinterpret_cast<PFN_vkCreatePipelineLayout>(vkGetInstanceProcAddr(instance,
                                                                                "vkCreatePipelineLayout"));
    vkDestroyPipelineLayout = reinterpret_cast<PFN_vkDestroyPipelineLayout>(vkGetInstanceProcAddr(instance,
                                                                                  "vkDestroyPipelineLayout"));
    vkCreateSampler = reinterpret_cast<PFN_vkCreateSampler>(vkGetInstanceProcAddr(instance, "vkCreateSampler"));
    vkDestroySampler = reinterpret_cast<PFN_vkDestroySampler>(vkGetInstanceProcAddr(instance, "vkDestroySampler"));
    vkCreateDescriptorSetLayout =
            reinterpret_cast<PFN_vkCreateDescriptorSetLayout>(vkGetInstanceProcAddr(instance,
                                                                    "vkCreateDescriptorSetLayout"));
    vkDestroyDescriptorSetLayout =
            reinterpret_cast<PFN_vkDestroyDescriptorSetLayout>(vkGetInstanceProcAddr(instance,
                                                                     "vkDestroyDescriptorSetLayout"));
    vkCreateDescriptorPool = reinterpret_cast<PFN_vkCreateDescriptorPool>(vkGetInstanceProcAddr(instance,
                                                                                "vkCreateDescriptorPool"));
    vkDestroyDescriptorPool = reinterpret_cast<PFN_vkDestroyDescriptorPool>(vkGetInstanceProcAddr(instance,
                                                                                  "vkDestroyDescriptorPool"));
    vkResetDescriptorPool = reinterpret_cast<PFN_vkResetDescriptorPool>(vkGetInstanceProcAddr(instance,
                                                                              "vkResetDescriptorPool"));
    vkAllocateDescriptorSets = reinterpret_cast<PFN_vkAllocateDescriptorSets>(vkGetInstanceProcAddr(instance,
                                                                                    "vkAllocateDescriptorSets"));
    vkFreeDescriptorSets = reinterpret_cast<PFN_vkFreeDescriptorSets>(vkGetInstanceProcAddr(instance,
                                                                            "vkFreeDescriptorSets"));
    vkUpdateDescriptorSets = reinterpret_cast<PFN_vkUpdateDescriptorSets>(vkGetInstanceProcAddr(instance,
                                                                                "vkUpdateDescriptorSets"));
    vkCreateFramebuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(vkGetInstanceProcAddr(instance,
                                                                          "vkCreateFramebuffer"));
    vkDestroyFramebuffer = reinterpret_cast<PFN_vkDestroyFramebuffer>(vkGetInstanceProcAddr(instance,
                                                                            "vkDestroyFramebuffer"));
    vkCreateRenderPass = reinterpret_cast<PFN_vkCreateRenderPass>(vkGetInstanceProcAddr(instance,
                                                                        "vkCreateRenderPass"));
    vkDestroyRenderPass = reinterpret_cast<PFN_vkDestroyRenderPass>(vkGetInstanceProcAddr(instance,
                                                                          "vkDestroyRenderPass"));
    vkGetRenderAreaGranularity = reinterpret_cast<PFN_vkGetRenderAreaGranularity>(vkGetInstanceProcAddr(instance,
                                                                                        "vkGetRenderAreaGranularity"));
    vkCreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(vkGetInstanceProcAddr(instance,
                                                                          "vkCreateCommandPool"));
    vkDestroyCommandPool = reinterpret_cast<PFN_vkDestroyCommandPool>(vkGetInstanceProcAddr(instance,
                                                                            "vkDestroyCommandPool"));
    vkResetCommandPool = reinterpret_cast<PFN_vkResetCommandPool>(vkGetInstanceProcAddr(instance,
                                                                        "vkResetCommandPool"));
    vkAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(vkGetInstanceProcAddr(instance,
                                                                                    "vkAllocateCommandBuffers"));
    vkFreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(vkGetInstanceProcAddr(instance,
                                                                            "vkFreeCommandBuffers"));
    vkBeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(vkGetInstanceProcAddr(instance,
                                                                            "vkBeginCommandBuffer"));
    vkEndCommandBuffer = reinterpret_cast<PFN_vkEndCommandBuffer>(vkGetInstanceProcAddr(instance,
                                                                        "vkEndCommandBuffer"));
    vkResetCommandBuffer = reinterpret_cast<PFN_vkResetCommandBuffer>(vkGetInstanceProcAddr(instance,
                                                                            "vkResetCommandBuffer"));
    vkCmdBindPipeline = reinterpret_cast<PFN_vkCmdBindPipeline>(vkGetInstanceProcAddr(instance,
                                                                      "vkCmdBindPipeline"));
    vkCmdSetViewport = reinterpret_cast<PFN_vkCmdSetViewport>(vkGetInstanceProcAddr(instance, "vkCmdSetViewport"));
    vkCmdSetScissor = reinterpret_cast<PFN_vkCmdSetScissor>(vkGetInstanceProcAddr(instance, "vkCmdSetScissor"));
    vkCmdSetLineWidth = reinterpret_cast<PFN_vkCmdSetLineWidth>(vkGetInstanceProcAddr(instance,
                                                                      "vkCmdSetLineWidth"));
    vkCmdSetDepthBias = reinterpret_cast<PFN_vkCmdSetDepthBias>(vkGetInstanceProcAddr(instance,
                                                                      "vkCmdSetDepthBias"));
    vkCmdSetBlendConstants = reinterpret_cast<PFN_vkCmdSetBlendConstants>(vkGetInstanceProcAddr(instance,
                                                                                "vkCmdSetBlendConstants"));
    vkCmdSetDepthBounds = reinterpret_cast<PFN_vkCmdSetDepthBounds>(vkGetInstanceProcAddr(instance,
                                                                          "vkCmdSetDepthBounds"));
    vkCmdSetStencilCompareMask = reinterpret_cast<PFN_vkCmdSetStencilCompareMask>(vkGetInstanceProcAddr(instance,
                                                                                        "vkCmdSetStencilCompareMask"));
    vkCmdSetStencilWriteMask = reinterpret_cast<PFN_vkCmdSetStencilWriteMask>(vkGetInstanceProcAddr(instance,
                                                                                    "vkCmdSetStencilWriteMask"));
    vkCmdSetStencilReference = reinterpret_cast<PFN_vkCmdSetStencilReference>(vkGetInstanceProcAddr(instance,
                                                                                    "vkCmdSetStencilReference"));
    vkCmdBindDescriptorSets = reinterpret_cast<PFN_vkCmdBindDescriptorSets>(vkGetInstanceProcAddr(instance,
                                                                                  "vkCmdBindDescriptorSets"));
    vkCmdBindIndexBuffer = reinterpret_cast<PFN_vkCmdBindIndexBuffer>(vkGetInstanceProcAddr(instance,
                                                                            "vkCmdBindIndexBuffer"));
    vkCmdBindVertexBuffers = reinterpret_cast<PFN_vkCmdBindVertexBuffers>(vkGetInstanceProcAddr(instance,
                                                                                "vkCmdBindVertexBuffers"));
    vkCmdDraw = reinterpret_cast<PFN_vkCmdDraw>(vkGetInstanceProcAddr(instance, "vkCmdDraw"));
    vkCmdDrawIndexed = reinterpret_cast<PFN_vkCmdDrawIndexed>(vkGetInstanceProcAddr(instance, "vkCmdDrawIndexed"));
    vkCmdDrawIndirect = reinterpret_cast<PFN_vkCmdDrawIndirect>(vkGetInstanceProcAddr(instance,
                                                                      "vkCmdDrawIndirect"));
    vkCmdDrawIndexedIndirect = reinterpret_cast<PFN_vkCmdDrawIndexedIndirect>(vkGetInstanceProcAddr(instance,
                                                                                    "vkCmdDrawIndexedIndirect"));
    vkCmdDispatch = reinterpret_cast<PFN_vkCmdDispatch>(vkGetInstanceProcAddr(instance, "vkCmdDispatch"));
    vkCmdDispatchIndirect = reinterpret_cast<PFN_vkCmdDispatchIndirect>(vkGetInstanceProcAddr(instance,
                                                                              "vkCmdDispatchIndirect"));
    vkCmdCopyBuffer = reinterpret_cast<PFN_vkCmdCopyBuffer>(vkGetInstanceProcAddr(instance, "vkCmdCopyBuffer"));
    vkCmdCopyImage = reinterpret_cast<PFN_vkCmdCopyImage>(vkGetInstanceProcAddr(instance, "vkCmdCopyImage"));
    vkCmdBlitImage = reinterpret_cast<PFN_vkCmdBlitImage>(vkGetInstanceProcAddr(instance, "vkCmdBlitImage"));
    vkCmdCopyBufferToImage = reinterpret_cast<PFN_vkCmdCopyBufferToImage>(vkGetInstanceProcAddr(instance,
                                                                                "vkCmdCopyBufferToImage"));
    vkCmdCopyImageToBuffer = reinterpret_cast<PFN_vkCmdCopyImageToBuffer>(vkGetInstanceProcAddr(instance,
                                                                                "vkCmdCopyImageToBuffer"));
    vkCmdUpdateBuffer = reinterpret_cast<PFN_vkCmdUpdateBuffer>(vkGetInstanceProcAddr(instance,
                                                                      "vkCmdUpdateBuffer"));
    vkCmdFillBuffer = reinterpret_cast<PFN_vkCmdFillBuffer>(vkGetInstanceProcAddr(instance, "vkCmdFillBuffer"));
    vkCmdClearColorImage = reinterpret_cast<PFN_vkCmdClearColorImage>(vkGetInstanceProcAddr(instance,
                                                                            "vkCmdClearColorImage"));
    vkCmdClearDepthStencilImage =
            reinterpret_cast<PFN_vkCmdClearDepthStencilImage>(vkGetInstanceProcAddr(instance,
                                                                    "vkCmdClearDepthStencilImage"));
    vkCmdClearAttachments = reinterpret_cast<PFN_vkCmdClearAttachments>(vkGetInstanceProcAddr(instance,
                                                                              "vkCmdClearAttachments"));
    vkCmdResolveImage = reinterpret_cast<PFN_vkCmdResolveImage>(vkGetInstanceProcAddr(instance,
                                                                      "vkCmdResolveImage"));
    vkCmdSetEvent = reinterpret_cast<PFN_vkCmdSetEvent>(vkGetInstanceProcAddr(instance, "vkCmdSetEvent"));
    vkCmdResetEvent = reinterpret_cast<PFN_vkCmdResetEvent>(vkGetInstanceProcAddr(instance, "vkCmdResetEvent"));
    vkCmdWaitEvents = reinterpret_cast<PFN_vkCmdWaitEvents>(vkGetInstanceProcAddr(instance, "vkCmdWaitEvents"));
    vkCmdPipelineBarrier = reinterpret_cast<PFN_vkCmdPipelineBarrier>(vkGetInstanceProcAddr(instance,
                                                                            "vkCmdPipelineBarrier"));
    vkCmdBeginQuery = reinterpret_cast<PFN_vkCmdBeginQuery>(vkGetInstanceProcAddr(instance, "vkCmdBeginQuery"));
    vkCmdEndQuery = reinterpret_cast<PFN_vkCmdEndQuery>(vkGetInstanceProcAddr(instance, "vkCmdEndQuery"));
    vkCmdResetQueryPool = reinterpret_cast<PFN_vkCmdResetQueryPool>(vkGetInstanceProcAddr(instance,
                                                                          "vkCmdResetQueryPool"));
    vkCmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(vkGetInstanceProcAddr(instance,
                                                                          "vkCmdWriteTimestamp"));
    vkCmdCopyQueryPoolResults = reinterpret_cast<PFN_vkCmdCopyQueryPoolResults>(vkGetInstanceProcAddr(instance,
                                                                                      "vkCmdCopyQueryPoolResults"));
    vkCmdPushConstants = reinterpret_cast<PFN_vkCmdPushConstants>(vkGetInstanceProcAddr(instance,
                                                                        "vkCmdPushConstants"));
    vkCmdBeginRenderPass = reinterpret_cast<PFN_vkCmdBeginRenderPass>(vkGetInstanceProcAddr(instance,
                                                                            "vkCmdBeginRenderPass"));
    vkCmdNextSubpass = reinterpret_cast<PFN_vkCmdNextSubpass>(vkGetInstanceProcAddr(instance, "vkCmdNextSubpass"));
    vkCmdEndRenderPass = reinterpret_cast<PFN_vkCmdEndRenderPass>(vkGetInstanceProcAddr(instance,
                                                                        "vkCmdEndRenderPass"));
    vkCmdExecuteCommands = reinterpret_cast<PFN_vkCmdExecuteCommands>(vkGetInstanceProcAddr(instance,
                                                                            "vkCmdExecuteCommands"));
    vkDestroySurfaceKHR = reinterpret_cast<PFN_vkDestroySurfaceKHR>(vkGetInstanceProcAddr(instance,
                                                                          "vkDestroySurfaceKHR"));
    vkGetPhysicalDeviceSurfaceSupportKHR =
            reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(vkGetInstanceProcAddr(instance,
                                                                             "vkGetPhysicalDeviceSurfaceSupportKHR"));
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
            vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    vkGetPhysicalDeviceSurfaceFormatsKHR =
            reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(vkGetInstanceProcAddr(instance,
                                                                             "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    vkGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
            vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetInstanceProcAddr(instance,
                                                                            "vkCreateSwapchainKHR"));
    vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetInstanceProcAddr(instance,
                                                                              "vkDestroySwapchainKHR"));
    vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetInstanceProcAddr(instance,
                                                                                  "vkGetSwapchainImagesKHR"));
    vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetInstanceProcAddr(instance,
                                                                              "vkAcquireNextImageKHR"));
    vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetInstanceProcAddr(instance,
                                                                      "vkQueuePresentKHR"));
    vkGetPhysicalDeviceDisplayPropertiesKHR =
            reinterpret_cast<PFN_vkGetPhysicalDeviceDisplayPropertiesKHR>(vkGetInstanceProcAddr(instance,
                                                                                "vkGetPhysicalDeviceDisplayPropertiesKHR"));
    vkGetPhysicalDeviceDisplayPlanePropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR>(
            vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR"));
    vkGetDisplayPlaneSupportedDisplaysKHR =
            reinterpret_cast<PFN_vkGetDisplayPlaneSupportedDisplaysKHR>(vkGetInstanceProcAddr(instance,
                                                                              "vkGetDisplayPlaneSupportedDisplaysKHR"));
    vkGetDisplayModePropertiesKHR =
            reinterpret_cast<PFN_vkGetDisplayModePropertiesKHR>(vkGetInstanceProcAddr(instance,
                                                                      "vkGetDisplayModePropertiesKHR"));
    vkCreateDisplayModeKHR = reinterpret_cast<PFN_vkCreateDisplayModeKHR>(vkGetInstanceProcAddr(instance,
                                                                                "vkCreateDisplayModeKHR"));
    vkGetDisplayPlaneCapabilitiesKHR =
            reinterpret_cast<PFN_vkGetDisplayPlaneCapabilitiesKHR>(vkGetInstanceProcAddr(instance,
                                                                         "vkGetDisplayPlaneCapabilitiesKHR"));
    vkCreateDisplayPlaneSurfaceKHR =
            reinterpret_cast<PFN_vkCreateDisplayPlaneSurfaceKHR>(vkGetInstanceProcAddr(instance,
                                                                       "vkCreateDisplayPlaneSurfaceKHR"));
    vkCreateSharedSwapchainsKHR =
            reinterpret_cast<PFN_vkCreateSharedSwapchainsKHR>(vkGetInstanceProcAddr(instance,
                                                                    "vkCreateSharedSwapchainsKHR"));
    vkGetImageMemoryRequirements2 = reinterpret_cast<PFN_vkGetImageMemoryRequirements2KHR>(vkGetInstanceProcAddr(instance, "vkGetImageMemoryRequirements2KHR"));

    vkBindImageMemory2 =
            reinterpret_cast<PFN_vkBindImageMemory2KHR>(vkGetInstanceProcAddr(instance,
                                                                      "vkBindImageMemory2KHR"));
    vkCreateSamplerYcbcrConversion =
            reinterpret_cast<PFN_vkCreateSamplerYcbcrConversionKHR>(vkGetInstanceProcAddr(instance,
                                                           "vkCreateSamplerYcbcrConversionKHR"));
    vkDestroySamplerYcbcrConversion =
            reinterpret_cast<PFN_vkDestroySamplerYcbcrConversionKHR>(vkGetInstanceProcAddr(instance,
                                                                                          "vkDestroySamplerYcbcrConversion"));
    vkGetPhysicalDeviceFeatures2 =
            reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(vkGetInstanceProcAddr(instance,
                                                                                          "vkGetPhysicalDeviceFeatures2KHR"));

#ifdef VK_USE_PLATFORM_XLIB_KHR
    vkCreateXlibSurfaceKHR = reinterpret_cast<PFN_vkCreateXlibSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR"));
    vkGetPhysicalDeviceXlibPresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR>(
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR"));
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    vkCreateXcbSurfaceKHR = reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR"));
    vkGetPhysicalDeviceXcbPresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR>(
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR"));
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    vkCreateWaylandSurfaceKHR = reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR"));
    vkGetPhysicalDeviceWaylandPresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR>(
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR"));
#endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
    vkCreateAndroidSurfaceKHR = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(vkGetInstanceProcAddr(instance,
                                                                                      "vkCreateAndroidSurfaceKHR"));
    vkGetAndroidHardwareBufferPropertiesANDROID = reinterpret_cast<PFN_vkGetAndroidHardwareBufferPropertiesANDROID>(vkGetInstanceProcAddr(instance,
                                                                                      "vkGetAndroidHardwareBufferPropertiesANDROID"));
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));
    vkGetPhysicalDeviceWin32PresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR"));
#endif
}

void freeVulkanLibrary()
{
    dlclose(libVulkan);
}
