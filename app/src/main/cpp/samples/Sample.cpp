/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Gain
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

#include "Sample.h"
#include "../util/LogUtil.h"
#include "Sample_01_Triangle.h"
#include "Sample_02_Cube.h"
#include "Sample_03_Texture.h"
#include "Sample_04_YUVTexture.h"
#include "Sample_05_LUT.h"
#include "Sample_06_MultiLUT.h"
#include "Sample_07_Histogram.h"
#include "Sample_08_3DModel.h"
#include "Sample_09_3DModelWithAnim.h"
#include "Sample_10_PBR.h"
#include "Sample_11_YUVTexture_VK_Conversion.h"
#include "includes/cube_data.h"
#include "jni.h"
#include "vulkan_wrapper.h"
#include <VulkanContextBase.h>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

#include <chrono>
#include <thread>

std::unique_ptr<Sample> Sample::create(AAssetManager *assetManager, uint32_t type)
{
    auto sample = std::make_unique<Sample>(type);
    sample->initialize(true, assetManager);
    return std::move(sample);
}

Sample::Sample(uint32_t type) :
    mSampleType(type)
{}

void Sample::initialize(bool enableDebug, AAssetManager *assetManager)
{
    switch (mSampleType)
    {
        case SampleType::TRIANGLE: {
            mContext = std::make_unique<Sample_01_Triangle>();
            break;
        }
        case SampleType::CUBE: {
            mContext = std::make_unique<Sample_02_Cube>();
            break;
        }
        case SampleType::TEXTURE_BITMAP: {
            mContext = std::make_unique<Sample_03_Texture>();
            break;
        }
        case SampleType::TEXTURE_YUV:
        case SampleType::CAMERA_YUV: {
            mContext = std::make_unique<Sample_04_YUVTexture>();
            break;
        }
        case SampleType::LUT: {
            mContext = std::make_unique<Sample_05_LUT>();
            break;
        }
        case SampleType::MULTI_LUT: {
            mContext = std::make_unique<Sample_06_MultiLUT>();
            break;
        }
        case SampleType::HISTOGRAM: {
            mContext = std::make_unique<Sample_07_Histogram>();
            break;
        }
        case SampleType::LOAD_3D_MODEL: {
            mContext = std::make_unique<Sample_08_3DModel>();
            break;
        }
        case SampleType::LOAD_3D_MODEL_WITH_ANIM: {
            mContext = std::make_unique<Sample_09_3DModelWithAnim>();
            break;
        }
        case SampleType::LOAD_3D_MODEL_PBR: {
            mContext = std::make_unique<Sample_10_PBR>();
            break;
        }
        case SampleType::TEXTURE_YUV_VK_CONVERSION: {
            mContext = std::make_unique<Sample_11_YUVTexture_VK_Conversion>();
            break;
        }
    }

    const bool success = mContext->create(enableDebug, assetManager);
    assert(success);
}

void Sample::setWindow(ANativeWindow *window, uint32_t w, uint32_t h)
{
    // init swapchain
    mContext->connectSwapChain();
    mContext->setNativeWindow(window, w, h);
}

void Sample::prepare(JNIEnv *env)
{
    mContext->prepare(env);
}

void Sample::prepareBitmap(JNIEnv *env, jobject bitmap)
{
    Sample_03_Texture *textureContext = dynamic_cast<Sample_03_Texture *>(mContext.get());
    textureContext->setJBitmap(env, bitmap);

    mContext->prepare(env);
}

void Sample::prepareYUV(JNIEnv *env, uint8_t *yData, uint8_t *uData, uint8_t *vData, uint32_t w,
                        uint32_t h, uint32_t yStride, uint32_t uStride, uint32_t vStride,
                        uint32_t orientation)
{
    if (mSampleType == SampleType::MULTI_LUT)
    {
        Sample_06_MultiLUT *cameraContext = dynamic_cast<Sample_06_MultiLUT *>(mContext.get());
        cameraContext->setYUVImage(
            yData, uData, vData, w, h, yStride, uStride, vStride, orientation);
    }
    else if (mSampleType == SampleType::LUT)
    {
        Sample_05_LUT *cameraContext = dynamic_cast<Sample_05_LUT *>(mContext.get());
        cameraContext->setYUVImage(
            yData, uData, vData, w, h, yStride, uStride, vStride, orientation);
    }
    else
    {
        Sample_04_YUVTexture *cameraContext = dynamic_cast<Sample_04_YUVTexture *>(mContext.get());
        cameraContext->setYUVImage(
            yData, uData, vData, w, h, yStride, uStride, vStride, orientation);
    }

    mContext->prepare(env);
}

void Sample::prepareI420VkConversion(JNIEnv *env, uint8_t *data, uint32_t w, uint32_t h)
{
    Sample_11_YUVTexture_VK_Conversion *cameraContext = dynamic_cast<Sample_11_YUVTexture_VK_Conversion *>(mContext.get());
    cameraContext->setYUVImage(data, w, h);

    mContext->prepare(env);
}

void Sample::prepareHistogram(JNIEnv *env, uint8_t *yData, uint8_t *uData, uint8_t *vData,
                              uint32_t w, uint32_t h, uint32_t yStride, uint32_t uStride,
                              uint32_t vStride, uint32_t uPixelStride, uint32_t vPixelStride,
                              uint32_t orientation)
{
    if (mSampleType == SampleType::HISTOGRAM)
    {
        Sample_07_Histogram *cameraContext = dynamic_cast<Sample_07_Histogram *>(mContext.get());
        cameraContext->setYUVImage(yData,
                                   uData,
                                   vData,
                                   w,
                                   h,
                                   yStride,
                                   uStride,
                                   vStride,
                                   uPixelStride,
                                   vPixelStride,
                                   orientation);
    }

    mContext->prepare(env);
}

void Sample::prepareLUT(JNIEnv *env, jobject bitmap)
{
    Sample_05_LUT *lutContext = dynamic_cast<Sample_05_LUT *>(mContext.get());
    lutContext->setLUTImage(env, bitmap);
}

void Sample::prepareLUTs(JNIEnv *env, jobjectArray bitmapArray)
{
    Sample_06_MultiLUT *lutContext = dynamic_cast<Sample_06_MultiLUT *>(mContext.get());
    lutContext->setLUTImages(env, bitmapArray);
}

void Sample::updateLUTs(JNIEnv *jniEnv, uint32_t itemWidth, uint32_t startIndex, uint32_t drawCount,
                        uint32_t offset)
{
    Sample_06_MultiLUT *lutContext = dynamic_cast<Sample_06_MultiLUT *>(mContext.get());
    lutContext->updateLUTs(jniEnv, itemWidth, startIndex, drawCount, offset);
}

void Sample::updateSelectedIndex(JNIEnv *jniEnv, uint32_t index)
{
    Sample_06_MultiLUT *lutContext = dynamic_cast<Sample_06_MultiLUT *>(mContext.get());
    lutContext->updateSelectedIndex(jniEnv, index);
}

void Sample::prepareCameraTexture(JNIEnv *env)
{}

void Sample::prepare3dModel(JNIEnv *env, std::string filePath)
{
    Sample_08_3DModel *modelContext = dynamic_cast<Sample_08_3DModel *>(mContext.get());
    modelContext->set3DModelPath(filePath);

    mContext->prepare(env);
}

void Sample::prepare3dModelWithAnim(JNIEnv *env, std::string filePath)
{
    Sample_09_3DModelWithAnim *modelContext = dynamic_cast<Sample_09_3DModelWithAnim *>(mContext.get());
    modelContext->set3DModelPath(filePath);

    mContext->prepare(env);
}

void Sample::prepare3dModelPBR(JNIEnv *env, std::string filePath)
{
    Sample_10_PBR *modelContext = dynamic_cast<Sample_10_PBR *>(mContext.get());
    modelContext->set3DModelPath(filePath);

    mContext->prepare(env);
}

void Sample::prepareLongExposure(JNIEnv *env, uint8_t *yData, uint8_t *uData, uint8_t *vData,
                                 uint32_t w, uint32_t h, uint32_t yStride, uint32_t uStride,
                                 uint32_t vStride)
{}

void Sample::onTouchActionMove(float deltaX, float deltaY)
{
    mContext->onTouchActionMove(deltaX, deltaY);

    mContext->prepare(nullptr);
}

void Sample::render(bool loop)
{
    mLoopDraw = loop;
    do
    {
        mContext->draw();
    } while (mLoopDraw);
}

void Sample::stopLoopRender()
{
    mLoopDraw = false;
}

void Sample::unInit(JNIEnv *env)
{}