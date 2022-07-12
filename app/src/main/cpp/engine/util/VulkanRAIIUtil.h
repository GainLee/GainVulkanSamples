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

#ifndef GAINVULKANSAMPLE_VULKANRAIIUTIL_H
#define GAINVULKANSAMPLE_VULKANRAIIUTIL_H

#include "vulkan_wrapper.h"

namespace vks
{
// The following code defines RAII wrappers of Vulkan objects.
// The wrapper of Vk<Object> will be named as Vulkan<Object>, e.g. VkInstance -> VulkanInstance.

template <typename T_VkHandle>
class VulkanObjectBase
{
  public:
	VulkanObjectBase() = default;

	// Disallow copy semantics to ensure the Vulkan object can only be freed once.
	VulkanObjectBase(const VulkanObjectBase &)            = delete;
	VulkanObjectBase &operator=(const VulkanObjectBase &) = delete;

	// Move semantics to remove access to the Vulkan object from the wrapper object
	// that is being moved. This ensures the Vulkan object will be freed only once.
	VulkanObjectBase(VulkanObjectBase &&other)
	{
		*this = std::move(other);
	}
	VulkanObjectBase &operator=(VulkanObjectBase &&other)
	{
		mHandle       = other.mHandle;
		other.mHandle = VK_NULL_HANDLE;
		return *this;
	}

	T_VkHandle handle() const
	{
		return mHandle;
	}
	T_VkHandle *pHandle()
	{
		return &mHandle;
	}

  protected:
	T_VkHandle mHandle = VK_NULL_HANDLE;
};

template <typename T_VkHandle, typename T_Destroyer>
class VulkanGlobalObject : public VulkanObjectBase<T_VkHandle>
{
  public:
	~VulkanGlobalObject()
	{
		if (this->mHandle != VK_NULL_HANDLE)
		{
			T_Destroyer::destroy(this->mHandle);
		}
	}
};

#define VULKAN_RAII_OBJECT(Type, destroyer)  \
	struct Vulkan##Type##Destroyer           \
	{                                        \
		static void destroy(Vk##Type handle) \
		{                                    \
			destroyer(handle, nullptr);      \
		}                                    \
	};                                       \
	using Vulkan##Type = VulkanGlobalObject<Vk##Type, Vulkan##Type##Destroyer>;

VULKAN_RAII_OBJECT(Instance, vkDestroyInstance);
VULKAN_RAII_OBJECT(Device, vkDestroyDevice);

#undef VULKAN_RAII_OBJECT

// Vulkan objects that is created/allocated with an instance
template <typename T_VkHandle, typename T_Destroyer>
class VulkanObjectFromInstance : public VulkanObjectBase<T_VkHandle>
{
  public:
	explicit VulkanObjectFromInstance(VkInstance instance) :
	    mInstance(instance)
	{}
	VulkanObjectFromInstance(VulkanObjectFromInstance &&other)
	{
		*this = std::move(other);
	}
	VulkanObjectFromInstance &operator=(VulkanObjectFromInstance &&other)
	{
		VulkanObjectBase<T_VkHandle>::operator=(std::move(other));
		mInstance = other.mInstance;
		return *this;
	}
	~VulkanObjectFromInstance()
	{
		if (this->mHandle != VK_NULL_HANDLE)
		{
			T_Destroyer::destroy(mInstance, this->mHandle);
		}
	}

  protected:
	VkInstance mInstance = VK_NULL_HANDLE;
};

// Vulkan objects that is created/allocated with a device
template <typename T_VkHandle, typename T_Destroyer>
class VulkanObjectFromDevice : public VulkanObjectBase<T_VkHandle>
{
  public:
	explicit VulkanObjectFromDevice(VkDevice device) :
	    mDevice(device)
	{}
	VulkanObjectFromDevice(VulkanObjectFromDevice &&other)
	{
		*this = std::move(other);
	}
	VulkanObjectFromDevice &operator=(VulkanObjectFromDevice &&other)
	{
		VulkanObjectBase<T_VkHandle>::operator=(std::move(other));
		mDevice = other.mDevice;
		return *this;
	}
	~VulkanObjectFromDevice()
	{
		if (this->mHandle != VK_NULL_HANDLE)
		{
			T_Destroyer::destroy(mDevice, this->mHandle);
		}
	}

  protected:
	VkDevice mDevice = VK_NULL_HANDLE;
};

#define VULKAN_RAII_OBJECT_FROM_DEVICE(Type, destroyer)       \
	struct Vulkan##Type##Destroyer                            \
	{                                                         \
		static void destroy(VkDevice device, Vk##Type handle) \
		{                                                     \
			destroyer(device, handle, nullptr);               \
		}                                                     \
	};                                                        \
	using Vulkan##Type = VulkanObjectFromDevice<Vk##Type, Vulkan##Type##Destroyer>;

VULKAN_RAII_OBJECT_FROM_DEVICE(CommandPool, vkDestroyCommandPool);
VULKAN_RAII_OBJECT_FROM_DEVICE(DescriptorPool, vkDestroyDescriptorPool);
VULKAN_RAII_OBJECT_FROM_DEVICE(Buffer, vkDestroyBuffer);
VULKAN_RAII_OBJECT_FROM_DEVICE(DeviceMemory, vkFreeMemory);
VULKAN_RAII_OBJECT_FROM_DEVICE(DescriptorSetLayout, vkDestroyDescriptorSetLayout);
VULKAN_RAII_OBJECT_FROM_DEVICE(PipelineLayout, vkDestroyPipelineLayout);
VULKAN_RAII_OBJECT_FROM_DEVICE(ShaderModule, vkDestroyShaderModule);
VULKAN_RAII_OBJECT_FROM_DEVICE(Pipeline, vkDestroyPipeline);
VULKAN_RAII_OBJECT_FROM_DEVICE(PipelineCache, vkDestroyPipelineCache);
VULKAN_RAII_OBJECT_FROM_DEVICE(Image, vkDestroyImage);
VULKAN_RAII_OBJECT_FROM_DEVICE(Sampler, vkDestroySampler);
VULKAN_RAII_OBJECT_FROM_DEVICE(ImageView, vkDestroyImageView);
VULKAN_RAII_OBJECT_FROM_DEVICE(Semaphore, vkDestroySemaphore);
VULKAN_RAII_OBJECT_FROM_DEVICE(Fence, vkDestroyFence);

#undef VULKAN_RAII_OBJECT_FROM_DEVICE

// Vulkan objects that is allocated from a pool
template <typename T_VkHandle, typename T_VkPoolHandle, typename T_Destroyer>
class VulkanObjectFromPool : public VulkanObjectBase<T_VkHandle>
{
  public:
	VulkanObjectFromPool(VkDevice device, T_VkPoolHandle pool) :
	    mDevice(device), mPool(pool)
	{}
	VulkanObjectFromPool(VulkanObjectFromPool &&other)
	{
		*this = std::move(other);
	}
	VulkanObjectFromPool &operator=(VulkanObjectFromPool &&other)
	{
		VulkanObjectBase<T_VkHandle>::operator=(std::move(other));
		mDevice = other.mDevice;
		mPool   = other.mPool;
		return *this;
	}
	~VulkanObjectFromPool()
	{
		if (this->mHandle != VK_NULL_HANDLE)
		{
			T_Destroyer::destroy(mDevice, mPool, this->mHandle);
		}
	}

  protected:
	VkDevice       mDevice = VK_NULL_HANDLE;
	T_VkPoolHandle mPool   = VK_NULL_HANDLE;
};

#define VULKAN_RAII_OBJECT_FROM_POOL(Type, VkPoolType, destroyer)              \
	struct Vulkan##Type##Destroyer                                             \
	{                                                                          \
		static void destroy(VkDevice device, VkPoolType pool, Vk##Type handle) \
		{                                                                      \
			destroyer(device, pool, 1, &handle);                               \
		}                                                                      \
	};                                                                         \
	using Vulkan##Type = VulkanObjectFromPool<Vk##Type, VkPoolType, Vulkan##Type##Destroyer>;

VULKAN_RAII_OBJECT_FROM_POOL(CommandBuffer, VkCommandPool, vkFreeCommandBuffers);
VULKAN_RAII_OBJECT_FROM_POOL(DescriptorSet, VkDescriptorPool, vkFreeDescriptorSets);

#undef VULKAN_RAII_OBJECT_FROM_POOL

}        // namespace vks

#endif        // GAINVULKANSAMPLE_VULKANRAIIUTIL_H
