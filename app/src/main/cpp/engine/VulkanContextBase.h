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

#ifndef GAINVULKANSAMPLE_VULKANCONTEXTBASE_H
#define GAINVULKANSAMPLE_VULKANCONTEXTBASE_H

#include "VulkanDeviceWrapper.hpp"
#include "VulkanSwapChain.h"
#include "util/VulkanRAIIUtil.h"
#include <android/asset_manager_jni.h>
#include <android/bitmap.h>
#include <android/configuration.h>
#include <android/hardware_buffer_jni.h>
#include <memory>
#include <optional>
#include <vector>
#include <vulkan_wrapper.h>
#define GLM_FORCE_RADIANS
#include "VulkanImageWrapper.h"
#include "VulkanUIOverlay.h"
#include "camera.hpp"
#include <glm/glm.hpp>

struct YUVSinglePassImage
{
    uint8_t *data;
    uint32_t w;
    uint32_t h;
    uint32_t stride;
    uint32_t pixelStride = 0;
    uint32_t orientation;
};

class VulkanContextBase
{
  private:
    void getDeviceConfig();
    void createPipelineCache();
    void createSynchronizationPrimitives();
    void initSwapchain();
    void setupSwapChain();
    void createCommandBuffers();

  public:
    bool create(bool enableDebug, AAssetManager *assetManager);
    // Prefer VulkanContextBase::create
    VulkanContextBase() :
        mDescriptorPool(VK_NULL_HANDLE), mPipelineCache(VK_NULL_HANDLE), mDescriptorSetLayout(VK_NULL_HANDLE), mPipelineLayout(VK_NULL_HANDLE), mPipeline(VK_NULL_HANDLE), presentCompleteSemaphore(VK_NULL_HANDLE), renderCompleteSemaphore(VK_NULL_HANDLE)
    {}

    VulkanContextBase(const char *vertPath, const char *fragPath) :
        mDescriptorPool(VK_NULL_HANDLE), mPipelineCache(VK_NULL_HANDLE), mDescriptorSetLayout(VK_NULL_HANDLE), mPipelineLayout(VK_NULL_HANDLE), mPipeline(VK_NULL_HANDLE), presentCompleteSemaphore(VK_NULL_HANDLE), renderCompleteSemaphore(VK_NULL_HANDLE), vertFilePath(vertPath), fragFilePath(fragPath)
    {}

    virtual ~VulkanContextBase();

    struct Settings
    {
        /** @brief Enable UI overlay */
        bool overlay = true;
    } settings;

    AAssetManager *mAssetManager;

    uint32_t mScreenDensity;

    // Getters of the managed Vulkan objects
    const std::shared_ptr<vks::VulkanDeviceWrapper> deviceWrapper() const
    {
        return mDeviceWrapper;
    }
    VkDevice device() const
    {
        return mDeviceWrapper->logicalDevice;
    }
    VkQueue queue() const
    {
        return mGraphicsQueue;
    }
    VkCommandPool commandPool() const
    {
        return mDeviceWrapper->commandPool;
    }
    VkDescriptorPool descriptorPool() const
    {
        return mDescriptorPool.handle();
    }

    // Create a semaphore with the managed device.
    bool createSemaphore(VkSemaphore *semaphore) const;

    void connectSwapChain();

    void setNativeWindow(ANativeWindow *window, uint32_t width, uint32_t height);

    void setupRenderPass();

    void prepareVertices(bool useStagingBuffers, const void *data, size_t bufSize);

    virtual void onTouchActionMove(float deltaX, float deltaY);

    virtual void preparePipelines();

    virtual void prepareUniformBuffers();

    void setupDescriptorSet();

    virtual void buildCommandBuffers();

    virtual void prepare(JNIEnv *env);

    virtual void draw();

    virtual void setupDepthStencil();

    virtual void setupFrameBuffer();

    virtual void unInit(JNIEnv *env);

    VkPipelineShaderStageCreateInfo loadShader(const char *          shaderFilePath,
                                               VkShaderStageFlagBits stage);

  protected:
    // Initialization
    bool createInstance(bool enableDebug);
    bool pickPhysicalDeviceAndQueueFamily(VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT |
                                                                             VK_QUEUE_COMPUTE_BIT);
    bool createDevice(VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT |
                                                         VK_QUEUE_COMPUTE_BIT);

    uint32_t getQueueFamilyIndex(VkQueueFlagBits queueFlags) const;

    void initRAIIObjects();

    void initUIOverlay();

    /** Prepare the next frame for workload submission by acquiring the next swap
	 * chain image */
    void prepareFrame();
    /** @brief Presents the current image to the swap chain */
    void submitFrame();

    void updateOverlay();

    void drawUI(const VkCommandBuffer commandBuffer);

    // Instance
    uint32_t       mInstanceVersion = 0;
    VulkanInstance mInstance;

    // Device and queue
    std::shared_ptr<vks::VulkanDeviceWrapper> mDeviceWrapper;

    VkQueue mGraphicsQueue = VK_NULL_HANDLE;
    VkQueue mPresentQueue  = VK_NULL_HANDLE;

    // Pools
    VulkanDescriptorPool mDescriptorPool;

    // Vertex buffer and attributes
    std::unique_ptr<vks::Buffer> mVerticesBuffer;

    // Uniform buffer block object
    std::unique_ptr<vks::Buffer> mUniformBuffer;

    struct
    {
        glm::mat4 projectionMatrix;
        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
    } uboVS;

    Camera mCamera;

    VulkanPipelineCache mPipelineCache;

    struct
    {
        ANativeWindow *nativeWindow;
        int32_t        windowWidth;
        int32_t        windowHeight;
    } mWindow;

    VulkanSwapChain mSwapChain;

    struct
    {
        VkImage        image;
        VkDeviceMemory mem;
        VkImageView    view;
    } depthStencil;

    VkFormat depthFormat;

    const char *vertFilePath;
    const char *fragFilePath;

    std::vector<VkFramebuffer> frameBuffers;
    VkRenderPass               mRenderPass;

    VulkanDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorSet           mDescriptorSet;
    VulkanPipelineLayout      mPipelineLayout;
    VulkanPipeline            mPipeline;

    // Synchronization primitives
    // Synchronization is an important concept of Vulkan that OpenGL mostly hid
    // away. Getting this right is crucial to using Vulkan.

    // Semaphores
    // Used to coordinate operations within the graphics queue and ensure correct
    // command ordering
    VulkanSemaphore presentCompleteSemaphore;
    VulkanSemaphore renderCompleteSemaphore;

    // Fences
    // Used to check the completion of queue operations (e.g. command buffer
    // execution)
    std::vector<VulkanFence> waitFences;

    // Active frame buffer index
    uint32_t currentBuffer = 0;

    // Command buffers used for rendering
    std::vector<VulkanCommandBuffer> drawCmdBuffers;

    vks::UIOverlay UIOverlay;

    /** @brief Last frame time measured using a high performance timer (if
	 * available) */
    float frameTimer = 1.0f;
    // Defines a frame rate independent timer value clamped from -1.0...1.0
    // For use in animations, rotations, etc.
    float timer = 0.0f;
    // Multiplier for speeding up (or slowing down) the global timer
    float timerSpeed = 1.0f;

    // Frame counter to display fps
    uint32_t                                                    frameCounter = 0;
    uint32_t                                                    lastFPS      = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp;

    bool mPrepared = false;
};

#endif        // GAINVULKANSAMPLE_VULKANCONTEXTBASE_H
