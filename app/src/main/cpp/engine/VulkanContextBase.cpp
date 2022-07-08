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

#include "VulkanContextBase.h"
#include "../util/LogUtil.h"
#include "VulkanDebug.h"
#include "VulkanInitializers.hpp"
#include "includes/cube_data.h"
#include <array>
#include <cmath>
#include <optional>
#include <vulkan_wrapper.h>

bool VulkanContextBase::create(bool enableDebug, AAssetManager *assetManager)
{
    mAssetManager = assetManager;
    getDeviceConfig();
    bool ret = createInstance(enableDebug) && pickPhysicalDeviceAndQueueFamily() && createDevice();

    initRAIIObjects();

    initUIOverlay();

    return ret;
}

void VulkanContextBase::getDeviceConfig()
{
    // Screen density
    AConfiguration *config = AConfiguration_new();
    AConfiguration_fromAssetManager(config, mAssetManager);
    mScreenDensity = AConfiguration_getDensity(config);
    AConfiguration_delete(config);
}

void VulkanContextBase::initRAIIObjects()
{
    mDescriptorPool          = VulkanDescriptorPool(device());
    mPipelineCache           = VulkanPipelineCache(device());
    mDescriptorSetLayout     = VulkanDescriptorSetLayout(device());
    mPipelineLayout          = VulkanPipelineLayout(device());
    mPipeline                = VulkanPipeline(device());
    presentCompleteSemaphore = VulkanSemaphore(device());
    renderCompleteSemaphore  = VulkanSemaphore(device());
}

void VulkanContextBase::initUIOverlay()
{
    UIOverlay.deviceWrapper = deviceWrapper();
    UIOverlay.screenDensity = mScreenDensity;
    UIOverlay.assetManager  = mAssetManager;
    UIOverlay.queue         = mGraphicsQueue;
    UIOverlay.init();
}

bool VulkanContextBase::createInstance(bool enableDebug)
{
#ifdef __ANDROID__
    // This place is the first place for samples to use Vulkan APIs.
    // Here, we are going to open Vulkan.so on the device and retrieve function pointers using
    // vulkan_wrapper helper.
    if (!loadVulkanLibrary())
    {
        LOGCATE("Failied load Vulkan library!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
#endif

    // Required instance layers
    std::vector<const char *> instanceLayers;
    if (enableDebug)
    {
        instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
    }

    // Required instance extensions
    std::vector<const char *> instanceExtensions = {
        //            VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        //            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };
    if (enableDebug)
    {
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Create instance
    const VkApplicationInfo applicationDesc = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "GainVulkanSample",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion         = static_cast<uint32_t>(
            (VK_VERSION_MINOR(mInstanceVersion) >= 1) ? VK_API_VERSION_1_1 : VK_API_VERSION_1_0),
    };

    VkInstanceCreateInfo instanceDesc = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &applicationDesc,
        .enabledLayerCount       = static_cast<uint32_t>(instanceLayers.size()),
        .ppEnabledLayerNames     = instanceLayers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data(),
    };

    CALL_VK(vkCreateInstance(&instanceDesc, nullptr, mInstance.pHandle()));

    if (vks::debug::debugable)
    {
        vks::debug::setupDebugging(mInstance.handle());
    }

#ifdef __ANDROID__
    loadVulkanFunctions(mInstance.handle());
    LOGCATI("Loaded Vulkan APIs.");
#endif

    return true;
}

bool VulkanContextBase::pickPhysicalDeviceAndQueueFamily(VkQueueFlags requestedQueueTypes)
{
    uint32_t numDevices = 0;
    CALL_VK(vkEnumeratePhysicalDevices(mInstance.handle(), &numDevices, nullptr));
    std::vector<VkPhysicalDevice> devices(numDevices);
    CALL_VK(vkEnumeratePhysicalDevices(mInstance.handle(), &numDevices, devices.data()));

    for (auto device : devices)
    {
        uint32_t numQueueFamilies = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueueFamilies, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(numQueueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueueFamilies, queueFamilies.data());

        bool haveQf = false;
        for (uint32_t i = 0; i < queueFamilies.size(); i++)
        {
            if (queueFamilies[i].queueFlags & requestedQueueTypes)
            {
                haveQf = true;
                break;
            }
        }
        if (!haveQf)
            continue;
        mDeviceWrapper = std::make_shared<vks::VulkanDeviceWrapper>(device);
        break;
    }

    return true;
}

bool VulkanContextBase::createDevice(VkQueueFlags requestedQueueTypes)
{
    // Required device extensions
    // These extensions are required to import an AHardwareBuffer to Vulkan.
    std::vector<const char *> deviceExtensions = {
        /*
	    VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
	    VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME,
	    VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME,*/
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
    };

    VkPhysicalDeviceFeatures enabledFeatures{};

    mDeviceWrapper->createLogicalDevice(enabledFeatures, deviceExtensions, requestedQueueTypes);

    vkGetDeviceQueue(mDeviceWrapper->logicalDevice,
                     mDeviceWrapper->queueFamilyIndices.graphics,
                     0,
                     &mGraphicsQueue);

    mDeviceWrapper->getDepthFormat(depthFormat);

    return true;
}

void VulkanContextBase::createPipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    CALL_VK(vkCreatePipelineCache(
        device(), &pipelineCacheCreateInfo, nullptr, mPipelineCache.pHandle()));
}

bool VulkanContextBase::createSemaphore(VkSemaphore *semaphore) const
{
    if (semaphore == nullptr)
        return false;
    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    CALL_VK(
        vkCreateSemaphore(mDeviceWrapper->logicalDevice, &semaphoreCreateInfo, nullptr, semaphore));
    return true;
}

void VulkanContextBase::connectSwapChain()
{
    mSwapChain.connect(
        mInstance.handle(), mDeviceWrapper->physicalDevice, mDeviceWrapper->logicalDevice);
}

void VulkanContextBase::setNativeWindow(ANativeWindow *window, uint32_t width, uint32_t height)
{
    mWindow.nativeWindow = window;
    mWindow.windowWidth  = width;
    mWindow.windowHeight = height;

    mCamera.setPerspective(
        45.0f, (float) mWindow.windowWidth / (float) mWindow.windowHeight, 0.1f, 256.0f);
}

void VulkanContextBase::prepareVertices(bool useStagingBuffers, const void *data, size_t bufSize)
{
    // A note on memory management in Vulkan in general:
    //	This is a very complex topic and while it's fine for an example application to small
    // individual memory allocations that is not 	what should be done a real-world application, where
    // you should allocate large chunks of memory at once instead.

    // Setup vertices

    uint32_t vertexBufferSize = bufSize;

    if (useStagingBuffers)
    {
        // Static data like vertex and index buffer should be stored on the device memory
        // for optimal (and fastest) access by the GPU
        //
        // To achieve this we use so-called "staging buffers" :
        // - Create a buffer that's visible to the host (and can be mapped)
        // - Copy the data to this buffer
        // - Create another buffer that's local on the device (VRAM) with the same size
        // - Copy the data from the host to the device using a command buffer
        // - Delete the host visible (staging) buffer
        // - Use the device local buffers for rendering

        auto stagingBuffers = Buffer::create(
            mDeviceWrapper,
            vertexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        stagingBuffers->copyFrom(data);

        vks::debug::setDeviceMemoryName(mDeviceWrapper->logicalDevice, stagingBuffers->getMemoryHandle(), "VulkanContextBase-prepareVertices-stagingBuffers");

        mVerticesBuffer =
            Buffer::create(mDeviceWrapper,
                           vertexBufferSize,
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vks::debug::setDeviceMemoryName(mDeviceWrapper->logicalDevice, mVerticesBuffer->getMemoryHandle(), "VulkanContextBase-prepareVertices-mVerticesBuffer");

        // Buffer copies have to be submitted to a queue, so we need a command buffer for them
        // Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that
        // may be faster when doing lots of copies
        VulkanCommandBuffer copyCmd(device(), commandPool());
        assert(mDeviceWrapper->beginSingleTimeCommand(copyCmd.pHandle()));

        // Put buffer region copies into command buffer
        VkBufferCopy copyRegion = {};

        // Vertex buffer
        copyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(copyCmd.handle(),
                        stagingBuffers->getBufferHandle(),
                        mVerticesBuffer->getBufferHandle(),
                        1,
                        &copyRegion);

        // Flushing the command buffer will also submit it to the queue and uses a fence to ensure
        // that all commands have been executed before returning
        mDeviceWrapper->endAndSubmitSingleTimeCommand(copyCmd.handle(), mGraphicsQueue, false);
    }
    else
    {
        // Don't use staging
        // Create host-visible buffers only and use these for rendering. This is not advised and
        // will usually result in lower rendering performance

        mVerticesBuffer = Buffer::create(
            mDeviceWrapper,
            vertexBufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        mVerticesBuffer->copyFrom(data);

        vks::debug::setDeviceMemoryName(mDeviceWrapper->logicalDevice, mVerticesBuffer->getMemoryHandle(), "VulkanContextBase-prepareVertices-mVerticesBuffer-no-staging");
    }
}

void VulkanContextBase::setupRenderPass()
{
    std::array<VkAttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format         = mSwapChain.colorFormat;
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // Depth attachment
    attachments[1].format         = depthFormat;
    attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {};
    colorReference.attachment            = 0;
    colorReference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment            = 1;
    depthReference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription    = {};
    subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount    = 1;
    subpassDescription.pColorAttachments       = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount    = 0;
    subpassDescription.pInputAttachments       = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments    = nullptr;
    subpassDescription.pResolveAttachments     = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass    = 0;
    dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass   = 0;
    dependencies[1].dstSubpass   = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass          = 0;
    subpass_dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask       = 0;
    subpass_dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags     = 0;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount        = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments           = attachments.data();
    renderPassInfo.subpassCount           = 1;
    renderPassInfo.pSubpasses             = &subpassDescription;
    renderPassInfo.dependencyCount        = 1;        // static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies          = &subpass_dependency;

    CALL_VK(vkCreateRenderPass(device(), &renderPassInfo, nullptr, &mRenderPass));
}

VkPipelineShaderStageCreateInfo VulkanContextBase::loadShader(const char *          shaderFilePath,
                                                              VkShaderStageFlagBits stage)
{
    // Read shader file from asset.
    AAsset *shaderFile = AAssetManager_open(mAssetManager, shaderFilePath, AASSET_MODE_BUFFER);
    assert(shaderFile != nullptr);
    const size_t      shaderSize = AAsset_getLength(shaderFile);
    std::vector<char> shader(shaderSize);
    int               status = AAsset_read(shaderFile, shader.data(), shaderSize);
    AAsset_close(shaderFile);
    assert(status >= 0);

    // Create shader module.
    const VkShaderModuleCreateInfo shaderDesc = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .flags    = 0,
        .codeSize = shaderSize,
        .pCode    = reinterpret_cast<const uint32_t *>(shader.data()),
    };
    VkShaderModule shaderModule;
    CALL_VK(
        vkCreateShaderModule(mDeviceWrapper->logicalDevice, &shaderDesc, nullptr, &shaderModule));

    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage                           = stage;
    shaderStage.pName                           = "main";
    shaderStage.module                          = shaderModule;
    return shaderStage;
}

void VulkanContextBase::preparePipelines()
{}

void VulkanContextBase::setupDescriptorSet()
{}

void VulkanContextBase::prepareUniformBuffers()
{}

void VulkanContextBase::buildCommandBuffers()
{}

void VulkanContextBase::initSwapchain()
{
#if defined(_WIN32)
    swapChain.initSurface(windowInstance, window);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    mSwapChain.initSurface(mWindow.nativeWindow);
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
    swapChain.initSurface(view);
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
    swapChain.initSurface(dfb, surface);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    swapChain.initSurface(display, surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    swapChain.initSurface(connection, window);
#elif (defined(_DIRECT2DISPLAY) || defined(VK_USE_PLATFORM_HEADLESS_EXT))
    swapChain.initSurface(width, height);
#endif
}

void VulkanContextBase::setupSwapChain()
{
    mSwapChain.create(&mWindow.windowWidth, &mWindow.windowHeight);
}

void VulkanContextBase::createCommandBuffers()
{
    // Create one command buffer for each swap chain image and reuse for rendering
    for (int i = 0; i < mSwapChain.imageCount; ++i)
    {
        VulkanCommandBuffer         cmdBuffer = VulkanCommandBuffer(device(), commandPool());
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool        = deviceWrapper()->commandPool;
        cmdBufAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufAllocateInfo.commandBufferCount = 1;
        CALL_VK(vkAllocateCommandBuffers(device(), &cmdBufAllocateInfo, cmdBuffer.pHandle()));

        drawCmdBuffers.push_back(std::move(cmdBuffer));
    }
}

void VulkanContextBase::createSynchronizationPrimitives()
{
    // Wait fences to sync command buffer access
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (int i = 0; i < drawCmdBuffers.size(); ++i)
    {
        waitFences.emplace_back(VulkanFence(device()));
    }
    for (auto &fence : waitFences)
    {
        CALL_VK(vkCreateFence(device(), &fenceCreateInfo, nullptr, fence.pHandle()));
    }
}

void VulkanContextBase::setupDepthStencil()
{
    VkImageCreateInfo imageCI{};
    imageCI.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType             = VK_IMAGE_TYPE_2D;
    imageCI.format                = depthFormat;
    imageCI.extent.width          = mWindow.windowWidth;
    imageCI.extent.height         = mWindow.windowHeight;
    imageCI.extent.depth          = 1;
    imageCI.mipLevels             = 1;
    imageCI.arrayLayers           = 1;
    imageCI.samples               = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling                = VK_IMAGE_TILING_OPTIMAL;
    imageCI.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCI.queueFamilyIndexCount = 0;
    imageCI.pQueueFamilyIndices   = NULL;
    imageCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    imageCI.usage                 = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    CALL_VK(vkCreateImage(device(), &imageCI, nullptr, &depthStencil.image));
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device(), depthStencil.image, &memReqs);

    VkMemoryAllocateInfo memAllloc{};
    memAllloc.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllloc.allocationSize = memReqs.size;
    uint32_t memTypeIdx =
        mDeviceWrapper->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    memAllloc.memoryTypeIndex = memTypeIdx;
    CALL_VK(vkAllocateMemory(device(), &memAllloc, nullptr, &depthStencil.mem));
    CALL_VK(vkBindImageMemory(device(), depthStencil.image, depthStencil.mem, 0));

    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.image                           = depthStencil.image;
    imageViewCI.format                          = depthFormat;
    imageViewCI.subresourceRange.baseMipLevel   = 0;
    imageViewCI.subresourceRange.levelCount     = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount     = 1;
    imageViewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageViewCI.components.r                    = VK_COMPONENT_SWIZZLE_R;
    imageViewCI.components.g                    = VK_COMPONENT_SWIZZLE_G;
    imageViewCI.components.b                    = VK_COMPONENT_SWIZZLE_B;
    imageViewCI.components.a                    = VK_COMPONENT_SWIZZLE_A;
    imageViewCI.flags                           = 0;
    // Stencil aspect should only be set on depth + stencil formats
    // (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
    if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT)
    {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    CALL_VK(vkCreateImageView(device(), &imageViewCI, nullptr, &depthStencil.view));
}

void VulkanContextBase::setupFrameBuffer()
{
    VkImageView attachments[2];

    // Depth/Stencil attachment is the same for all frame buffers
    attachments[1] = depthStencil.view;

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext                   = NULL;
    frameBufferCreateInfo.renderPass              = mRenderPass;
    frameBufferCreateInfo.attachmentCount         = 2;
    frameBufferCreateInfo.pAttachments            = attachments;
    frameBufferCreateInfo.width                   = mWindow.windowWidth;
    frameBufferCreateInfo.height                  = mWindow.windowHeight;
    frameBufferCreateInfo.layers                  = 1;

    // Create frame buffers for every swap chain image
    frameBuffers.resize(mSwapChain.imageCount);
    for (uint32_t i = 0; i < frameBuffers.size(); i++)
    {
        attachments[0] = mSwapChain.buffers[i].view;
        CALL_VK(vkCreateFramebuffer(device(), &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
    }
}

void VulkanContextBase::prepare(JNIEnv *env)
{
    initSwapchain();
    setupSwapChain();
    createCommandBuffers();
    createSynchronizationPrimitives();
    setupDepthStencil();
    setupRenderPass();
    createPipelineCache();
    setupFrameBuffer();

    if (settings.overlay)
    {
        // Shaders
        UIOverlay.shaders = {
            loadShader("shaders/base/uioverlay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            loadShader("shaders/base/uioverlay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
        };
        UIOverlay.prepareResources();
        UIOverlay.preparePipeline(mPipelineCache.handle(), mRenderPass);

        // Shader modules are no longer needed once the graphics pipeline has been created
        vkDestroyShaderModule(device(), UIOverlay.shaders[0].module, nullptr);
        vkDestroyShaderModule(device(), UIOverlay.shaders[1].module, nullptr);
    }
}

// 1. Acquire Image，设置完成信号presentCompleteSemaphore
// 2. Queue Submit，等待完成信号presentCompleteSemaphore，并设置完成信号renderCompleteSemaphore
// 3. Present，等待完成信号renderCompleteSemaphore
void VulkanContextBase::draw()
{
    auto tStart = std::chrono::high_resolution_clock::now();

    prepareFrame();

    // Use a fence to wait until the command buffer has finished execution before using it again
    CALL_VK(vkWaitForFences(device(), 1, waitFences[currentBuffer].pHandle(), VK_TRUE, UINT64_MAX));
    CALL_VK(vkResetFences(device(), 1, waitFences[currentBuffer].pHandle()));

    // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // The submit info structure specifies a command buffer queue submission batch
    VkSubmitInfo submitInfo      = {};
    submitInfo.sType             = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &waitStageMask;        // Pointer to the list of pipeline stages that
                                                          // the semaphore waits will occur at
    submitInfo.pWaitSemaphores =
        presentCompleteSemaphore.pHandle();        // Semaphore(s) to wait upon before the submitted
                                                   // command buffer starts executing
    submitInfo.waitSemaphoreCount = 1;             // One wait semaphore
    submitInfo.pSignalSemaphores =
        renderCompleteSemaphore
            .pHandle();                         // Semaphore(s) to be signaled when command buffers have completed
    submitInfo.signalSemaphoreCount = 1;        // One signal semaphore
    submitInfo.pCommandBuffers =
        drawCmdBuffers[currentBuffer]
            .pHandle();                       // Command buffers(s) to execute in this batch (submission)
    submitInfo.commandBufferCount = 1;        // One command buffer

    // Submit to the graphics queue passing a wait fence
    CALL_VK(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, waitFences[currentBuffer].handle()));

    submitFrame();

    frameCounter++;
    auto tEnd  = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    frameTimer = tDiff / 1000.0f;

    float fpsTimer = std::chrono::duration<double, std::milli>(tEnd - lastTimestamp).count();

    if (fpsTimer > 1000.0f)
    {
        lastFPS       = (float) frameCounter * (1000.0f / fpsTimer);
        frameCounter  = 0;
        lastTimestamp = tEnd;
    }

    if (settings.overlay)
    {
        updateOverlay();
    }
}

void VulkanContextBase::updateOverlay()
{
    if (!settings.overlay)
        return;

    ImGuiIO &io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float) mWindow.windowWidth, (float) mWindow.windowHeight);
    io.DeltaTime   = frameTimer;

    /*io.MousePos = ImVec2(mousePos.x, mousePos.y);
	io.MouseDown[0] = mouseButtons.left;
	io.MouseDown[1] = mouseButtons.right;*/

    ImGui::NewFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
    ImGui::Begin(
        "GainVulkanSample",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::TextUnformatted("GainVulkanSample");
    ImGui::TextUnformatted(mDeviceWrapper->properties.deviceName);
    ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / lastFPS), lastFPS);

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 5.0f * UIOverlay.scale));
#endif
    ImGui::PushItemWidth(110.0f * UIOverlay.scale);
    ImGui::PopItemWidth();
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    ImGui::PopStyleVar();
#endif

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::Render();

    if (UIOverlay.update() || UIOverlay.updated)
    {
        buildCommandBuffers();
        UIOverlay.updated = false;
    }

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
/*    if (mouseButtons.left) {
	    mouseButtons.left = false;
	}*/
#endif
}

void VulkanContextBase::drawUI(const VkCommandBuffer commandBuffer)
{
    if (settings.overlay)
    {
        const VkViewport viewport = vks::initializers::viewport(
            (float) mWindow.windowWidth, (float) mWindow.windowHeight, 0.0f, 1.0f);
        const VkRect2D scissor =
            vks::initializers::rect2D(mWindow.windowWidth, mWindow.windowHeight, 0, 0);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        UIOverlay.draw(commandBuffer);
    }
}

void VulkanContextBase::prepareFrame()
{
    CALL_VK(mSwapChain.acquireNextImage(presentCompleteSemaphore.handle(), &currentBuffer));
}

void VulkanContextBase::submitFrame()
{
    // Present the current buffer to the swap chain
    // Pass the semaphore signaled by the command buffer submission from the submit info as the wait
    // semaphore for swap chain presentation This ensures that the image is not presented to the
    // windowing system until all commands have been submitted
    VkResult present =
        mSwapChain.queuePresent(mGraphicsQueue, currentBuffer, renderCompleteSemaphore.handle());
    if (!((present == VK_SUCCESS) || (present == VK_SUBOPTIMAL_KHR)))
    {
        CALL_VK(present);
    }
}

void VulkanContextBase::onTouchActionMove(float deltaX, float deltaY)
{
    mCamera.rotate(glm::vec3(deltaY * mCamera.rotationSpeed * 0.5f, 0.0f, 0.0f));
    mCamera.rotate(glm::vec3(0.0f, -deltaX * mCamera.rotationSpeed * 0.5f, 0.0f));
}

void VulkanContextBase::unInit(JNIEnv *env)
{}

VulkanContextBase::~VulkanContextBase()
{
    vkDeviceWaitIdle(device());

    mSwapChain.cleanup();

    vks::debug::freeDebugCallback(mInstance.handle());

    if (settings.overlay)
    {
        UIOverlay.freeResources();
    }
}