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

#ifndef GAINVULKANSAMPLE_SAMPLE_H
#define GAINVULKANSAMPLE_SAMPLE_H

#include "../engine/VulkanContextBase.h"
#include "../engine/VulkanResources.h"
#include <android/native_window_jni.h>
#include <glm/vec2.hpp>
#include <memory>
#include <vulkan_wrapper.h>

using namespace gain;

// 与PlaceholderContent中SampleType保持一致
enum SampleType
{
    TRIANGLE,
    CUBE,
    TEXTURE_BITMAP,
    TEXTURE_YUV,
    TEXTURE_YUV_VK_CONVERSION,
    CAMERA_YUV,
    CAMERA_HARDWAREBUFFER,
    LUT,
    MULTI_LUT,
    HISTOGRAM,
    LOAD_3D_MODEL,
    LOAD_3D_MODEL_WITH_ANIM,
    LOAD_3D_MODEL_PBR,
};

class Sample
{
  public:
    explicit Sample(uint32_t type);

    static std::unique_ptr<Sample> create(AAssetManager *assetManager, uint32_t type);

    void initialize(bool enableDebug, AAssetManager *assetManager);

    void unInit(JNIEnv *env);

    void prepare(JNIEnv *env);

    void prepareBitmap(JNIEnv *env, jobject bitmap);

    void prepareLUT(JNIEnv *env, jobject bitmap);

    void prepareLUTs(JNIEnv *env, jobjectArray bitmapArray);

    void updateLUTs(JNIEnv *jniEnv, uint32_t itemWidth, uint32_t startIndex, uint32_t drawCount, uint32_t offset);

    void updateSelectedIndex(JNIEnv *jniEnv, uint32_t index);

    void prepareCameraTexture(JNIEnv *env);

    void prepare3dModel(JNIEnv *env, std::string filePath);

    void prepare3dModelWithAnim(JNIEnv *env, std::string filePath);

    void prepare3dModelPBR(JNIEnv *env, std::string filePath);

    void prepareYUV(JNIEnv *env, uint8_t *yData, uint8_t *uData, uint8_t *vData, uint32_t w, uint32_t h, uint32_t yStride, uint32_t uStride, uint32_t vStride, uint32_t orientation = 0);

    void prepareI420VkConversion(JNIEnv *env, uint8_t *data, uint32_t w, uint32_t h);

    void prepareHistogram(JNIEnv *env, uint8_t *yData, uint8_t *uData, uint8_t *vData, uint32_t w, uint32_t h, uint32_t yStride, uint32_t uStride, uint32_t vStride, uint32_t uPixelStride, uint32_t vPixelStride, uint32_t orientation = 0);

    void prepareLongExposure(JNIEnv *env, uint8_t *yData, uint8_t *uData, uint8_t *vData, uint32_t w, uint32_t h, uint32_t yStride, uint32_t uStride, uint32_t vStride);

    void render(bool loop);

    void stopLoopRender();

    void setWindow(ANativeWindow *window, uint32_t w, uint32_t h);

    void onTouchActionMove(float deltaX, float deltaY);

  private:
    std::unique_ptr<VulkanContextBase> mContext;

    uint32_t mSampleType;

    bool mLoopDraw;
};

#endif        // GAINVULKANSAMPLE_SAMPLE_H
