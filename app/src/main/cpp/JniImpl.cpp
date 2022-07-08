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

#include "engine/util/LogUtil.h"
#include "engine/vulkan_wrapper/vulkan_wrapper.h"
#include "jni.h"
#include "samples/Sample.h"
#include <android/native_window_jni.h>
#include <stdexcept>
#include <vulkan/vulkan.h>

#define JCMCPRV(rettype, name) \
    extern "C" JNIEXPORT rettype JNICALL Java_com_gain_vulkan_NativeVulkan_##name

// TODO We do it by compute pipeline
void removeFakeUVData(uint8_t *srcBuffer, jint width, jint height, jint stride, jint pixelStride,
                      uint8_t *dstBuffer)
{
    int index = 0;
    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            dstBuffer[index++] = srcBuffer[row * stride + col * pixelStride];
        }
    }
}

Sample *castToSample(jlong handle)
{
    return reinterpret_cast<Sample *>(static_cast<uintptr_t>(handle));
}

JCMCPRV(jlong, nativeInit)
(JNIEnv *env, jobject thiz, jobject asset_manager, jint type)
{
    auto *assetManager = AAssetManager_fromJava(env, asset_manager);
    assert(assetManager != nullptr);
    auto sample = Sample::create(assetManager, type);
    return static_cast<jlong>(reinterpret_cast<uintptr_t>(sample.release()));
}

JCMCPRV(void, nativeUnInit)
(JNIEnv *env, jobject thiz, jlong handle)
{
    if (handle == 0L)
    {
        return;
    }
    auto sample = castToSample(handle);
    sample->unInit(env);
    delete sample;
}

JCMCPRV(void, nativePrepare)
(JNIEnv *env, jobject thiz, jlong handle)
{
    castToSample(handle)->prepare(env);
}

JCMCPRV(void, nativePrepareBitmap)
(JNIEnv *env, jobject thiz, jlong handle, jobject bitmap)
{
    castToSample(handle)->prepareBitmap(env, bitmap);
}

JCMCPRV(void, nativePrepareI420)
(JNIEnv *env, jobject thiz, jlong handle, jbyteArray img_data, jint w, jint h, jint stride_y,
 jint stride_u, jint stride_v)
{
    uint8_t *buf = reinterpret_cast<uint8_t *>(env->GetByteArrayElements(img_data, JNI_FALSE));
    castToSample(handle)->prepareYUV(env,
                                     buf,
                                     buf + stride_y * h,
                                     buf + stride_y * h + stride_u * h / 2,
                                     w,
                                     h,
                                     stride_y,
                                     stride_u,
                                     stride_v);
    env->ReleaseByteArrayElements(img_data, reinterpret_cast<jbyte *>(buf), 0);
}

JCMCPRV(void, nativePrepareI420VkConversion)
(JNIEnv *env, jobject thiz, jlong handle, jbyteArray img_data, jint w, jint h, jint stride_y,
 jint stride_u, jint stride_v)
{
    uint8_t *buf = reinterpret_cast<uint8_t *>(env->GetByteArrayElements(img_data, JNI_FALSE));
    castToSample(handle)->prepareI420VkConversion(env,
                                                  buf,
                                                  w,
                                                  h);
    env->ReleaseByteArrayElements(img_data, reinterpret_cast<jbyte *>(buf), 0);
}

JCMCPRV(void, nativePrepareCameraYUV)
(JNIEnv *env, jobject thiz, jlong handle, jobject y_buffer, jobject u_buffer, jobject v_buffer,
 jint w, jint h, jint stride_y, jint stride_u, jint stride_v, jint uPixelStride, jint vPixelStride,
 jint orientation)
{
    uint8_t *y = static_cast<uint8_t *>(env->GetDirectBufferAddress(y_buffer));
    removeFakeUVData(y, w, h, stride_y, 1, y);
    uint8_t *u    = static_cast<uint8_t *>(env->GetDirectBufferAddress(u_buffer));
    uint8_t *dstU = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * w * h / 4));
    removeFakeUVData(u, w / 2, h / 2, stride_u, uPixelStride, dstU);
    uint8_t *v    = static_cast<uint8_t *>(env->GetDirectBufferAddress(v_buffer));
    uint8_t *dstV = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * w * h / 4));
    removeFakeUVData(v, w / 2, h / 2, stride_v, vPixelStride, dstV);
    castToSample(handle)->prepareYUV(env, y, dstU, dstV, w, h, w, w / 2, w / 2, orientation);
}

JCMCPRV(void, nativePrepareHistogram)
(JNIEnv *env, jobject thiz, jlong handle, jobject y_buffer, jobject u_buffer, jobject v_buffer,
 jint w, jint h, jint stride_y, jint stride_u, jint stride_v, jint uPixelStride, jint vPixelStride,
 jint orientation)
{
    uint8_t *y = static_cast<uint8_t *>(env->GetDirectBufferAddress(y_buffer));
    uint8_t *u = static_cast<uint8_t *>(env->GetDirectBufferAddress(u_buffer));
    uint8_t *v = static_cast<uint8_t *>(env->GetDirectBufferAddress(v_buffer));
    castToSample(handle)->prepareHistogram(
        env, y, u, v, w, h, stride_y, stride_u, stride_v, uPixelStride, vPixelStride, orientation);
}

JCMCPRV(void, nativePrepareCameraTexture)
(JNIEnv *env, jobject thiz, jlong handle)
{
    castToSample(handle)->prepareCameraTexture(env);
}

JCMCPRV(void, nativePrepareLUT)
(JNIEnv *env, jobject thiz, jlong handle, jobject lut_bitmap)
{
    castToSample(handle)->prepareLUT(env, lut_bitmap);
}

JCMCPRV(void, nativePrepareLUTs)
(JNIEnv *env, jobject thiz, jlong handle, jobjectArray luts)
{
    castToSample(handle)->prepareLUTs(env, luts);
}

JCMCPRV(void, nativeUpdateLUTs)
(JNIEnv *env, jobject thiz, jlong handle, jint item_width, jint start_index, jint draw_count,
 jint offset)
{
    castToSample(handle)->updateLUTs(env, item_width, start_index, draw_count, offset);
}

JCMCPRV(void, nativeUpdateSelectedIndex)
(JNIEnv *env, jobject thiz, jlong handle, jint index)
{
    castToSample(handle)->updateSelectedIndex(env, index);
}

JCMCPRV(void, nativePrepareLongExposure)
(JNIEnv *env, jobject thiz, jlong handle, jobject y_buffer, jobject u_buffer, jobject v_buffer,
 jint w, jint h, jint stride_y, jint stride_u, jint stride_v)
{
    uint8_t *y = static_cast<uint8_t *>(env->GetDirectBufferAddress(y_buffer));
    uint8_t *u = static_cast<uint8_t *>(env->GetDirectBufferAddress(u_buffer));
    uint8_t *v = static_cast<uint8_t *>(env->GetDirectBufferAddress(v_buffer));
    castToSample(handle)->prepareLongExposure(env, y, u, v, w, h, stride_y, stride_u, stride_v);
}

JCMCPRV(void, nativePrepare3dModel)
(JNIEnv *env, jobject thiz, jlong handle, jstring filePath)
{
    const char *chars = env->GetStringUTFChars(filePath, NULL);
    castToSample(handle)->prepare3dModel(env, chars);

    env->ReleaseStringUTFChars(filePath, chars);
}

JCMCPRV(void, nativePrepare3dModelWithAnim)
(JNIEnv *env, jobject thiz, jlong handle, jstring filePath)
{
    const char *chars = env->GetStringUTFChars(filePath, NULL);
    castToSample(handle)->prepare3dModelWithAnim(env, chars);

    env->ReleaseStringUTFChars(filePath, chars);
}

JCMCPRV(void, nativePrepare3dModelPBR)
(JNIEnv *env, jobject thiz, jlong handle, jstring filePath)
{
    const char *chars = env->GetStringUTFChars(filePath, NULL);
    castToSample(handle)->prepare3dModelPBR(env, chars);

    env->ReleaseStringUTFChars(filePath, chars);
}

JCMCPRV(void, nativeStartRender)
(JNIEnv *env, jobject thiz, jlong handle, jboolean loop)
{
    castToSample(handle)->render(loop);
}

JCMCPRV(void, native_1setWindow)
(JNIEnv *env, jobject thiz, jlong handle, jobject surface, jint width, jint height)
{
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    castToSample(handle)->setWindow(window, width, height);
}

JCMCPRV(void, nativeOnTouchActionMove)
(JNIEnv *env, jobject thiz, jlong handle, jfloat delta_x, jfloat delta_y)
{
    castToSample(handle)->onTouchActionMove(delta_x, delta_y);
}

JCMCPRV(void, nativeStopLoopRender)
(JNIEnv *env, jobject thiz, jlong handle)
{
    castToSample(handle)->stopLoopRender();
}