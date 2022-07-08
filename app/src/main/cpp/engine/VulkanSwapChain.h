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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "../util/LogUtil.h"
#include "../vulkan_wrapper/vulkan_wrapper.h"

typedef struct _SwapChainBuffers
{
    VkImage     image;
    VkImageView view;
} SwapChainBuffer;

class VulkanSwapChain
{
  private:
    VkInstance       instance;
    VkDevice         device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR     surface;

  public:
    VkFormat                     colorFormat;
    VkColorSpaceKHR              colorSpace;
    VkSwapchainKHR               swapChain = VK_NULL_HANDLE;
    uint32_t                     imageCount;
    std::vector<VkImage>         images;
    std::vector<SwapChainBuffer> buffers;
    uint32_t                     queueNodeIndex = UINT32_MAX;

    void     initSurface(ANativeWindow *window);
    void     connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
    void     create(int32_t *width, int32_t *height, bool vsync = false);
    VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex);
    VkResult queuePresent(VkQueue queue, uint32_t imageIndex,
                          VkSemaphore waitSemaphore = VK_NULL_HANDLE);
    void     cleanup();
};
