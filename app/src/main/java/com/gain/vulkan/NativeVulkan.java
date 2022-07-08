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
package com.gain.vulkan;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.jetbrains.annotations.NotNull;

import java.nio.ByteBuffer;

public class NativeVulkan implements VulkanSample {
    // Used to load the 'vulkan' library on application startup.
    static {
        System.loadLibrary("vulkanSample");
    }

    private long mVulkanHandle;

    private HandlerThread mRenderThread;
    private Handler mRenderHandler;

    private boolean mDrawing = false;

    // Return a non-zero handle on success, and 0L if failed.
    private native long nativeInit(AssetManager assetManager, int sampleType);

    // Frees up any underlying native resources. After calling this method, the NativeVulkan
    // must not be used in any way.
    private native void nativeUnInit(long handle);

    private native void nativePrepare(long handle);

    private native void nativePrepareBitmap(long handle, Bitmap bitmap);

    private native void nativePrepareI420(long handle, @NonNull byte[] imgData, int w, int h, int strideY, int strideU, int strideV);

    private native void nativePrepareI420VkConversion(long handle, @NonNull byte[] imgData, int w, int h, int strideY, int strideU, int strideV);

    private native void nativePrepareCameraYUV(long handle,
                                         @NonNull ByteBuffer yBuffer,
                                         @NonNull ByteBuffer uBuffer,
                                         @NonNull ByteBuffer vBuffer,
                                         int w, int h,
                                         int strideY,
                                         int strideU,
                                         int strideV,
                                         int uPixelstride,
                                         int vPixelstride,
                                         int orientation);

    private native void nativePrepareHistogram(long handle,
                                               @NonNull ByteBuffer yBuffer,
                                               @NonNull ByteBuffer uBuffer,
                                               @NonNull ByteBuffer vBuffer,
                                               int w, int h,
                                               int strideY,
                                               int strideU,
                                               int strideV,
                                               int uPixelstride,
                                               int vPixelstride,
                                               int orientation);

    private native void nativePrepareCameraTexture(long handle);

    private native void nativePrepareLUT(long handle, Bitmap lutBitmap);

    private native void nativePrepareLUTs(long handle, Bitmap[] luts);

    private native void nativeUpdateLUTs(long handle, int itemWidth, int startIndex, int drawCount, int offset);

    private native void nativeUpdateSelectedIndex(long handle, int selectedIndex);

    private native void nativePrepareLongExposure(long handle, @NonNull ByteBuffer yBuffer, @NonNull ByteBuffer uBuffer, @NonNull ByteBuffer vBuffer, int w, int h, int strideY, int strideU, int strideV);

    private native void nativePrepare3dModel(long handle, String filePath);

    private native void nativePrepare3dModelPBR(long handle, String filePath);

    private native void nativePrepare3dModelWithAnim(long handle, String filePath);

    private native void nativeStartRender(long handle, boolean loop);

    private native void nativeStopLoopRender(long handle);

    private native void native_setWindow(long handle, Surface surface, int width, int height);

    private native void nativeOnTouchActionMove(long handle, float deltaX, float deltaY);

    @Override
    public void init(AssetManager assetManager, int sampleType) {
        if (mRenderThread != null) {
            mRenderThread.quitSafely();
            mRenderThread = null;
            mRenderHandler = null;
        }

        mRenderThread = new HandlerThread("VK_RenderThread");
        mRenderThread.start();
        mRenderHandler = new Handler(mRenderThread.getLooper());

        mVulkanHandle = nativeInit(assetManager, sampleType);
    }

    @Override
    public void unInit() {
        if (mVulkanHandle != 0L) {
            nativeUnInit(mVulkanHandle);
            mVulkanHandle = 0L;
        }

        if (mRenderThread != null) {
            mRenderThread.quitSafely();
            mRenderThread = null;
            mRenderHandler = null;
        }

        mDrawing = false;
    }

    @Override
    public void setWindow(@Nullable Surface surface, int width, int height) {
        native_setWindow(mVulkanHandle, surface, width, height);
    }

    @Override
    public void prepare() {
        nativePrepare(mVulkanHandle);
    }

    @Override
    public void prepareBitmap(@NonNull Bitmap bitmap) {
        nativePrepareBitmap(mVulkanHandle, bitmap);
    }

    @Override
    public void prepareI420(@NonNull byte[] imgData, int w, int h, int strideY, int strideU, int strideV) {
        nativePrepareI420(mVulkanHandle, imgData, w, h, strideY, strideU, strideV);
    }

    @Override
    public void prepareI420VkConversion(@NonNull byte[] imgData, int w, int h, int strideY, int strideU, int strideV) {
        nativePrepareI420VkConversion(mVulkanHandle, imgData, w, h, strideY, strideU, strideV);
    }

    @Override
    public void prepareYUV(@NonNull ByteBuffer yBuffer,
                          @NonNull ByteBuffer uBuffer,
                          @NonNull ByteBuffer vBuffer,
                          int w, int h,
                          int strideY,
                          int strideU,
                          int strideV,
                          int uPixelstride,
                          int vPixelstride,
                          int orientation) {
        nativePrepareCameraYUV(mVulkanHandle, yBuffer, uBuffer, vBuffer, w, h, strideY, strideU, strideV, uPixelstride, vPixelstride, orientation);
    }

    @Override
    public void prepareHistogram(@NonNull ByteBuffer yBuffer, @NonNull ByteBuffer uBuffer, @NonNull ByteBuffer vBuffer, int w, int h, int strideY, int strideU, int strideV, int uPixelstride, int vPixelstride, int orientation) {
        nativePrepareHistogram(mVulkanHandle, yBuffer, uBuffer, vBuffer, w, h, strideY, strideU, strideV, uPixelstride, vPixelstride, orientation);
    }

    @Override
    public void prepareCameraTexture() {
        nativePrepareCameraTexture(mVulkanHandle);
    }

    @Override
    public void prepareLUT(@NonNull Bitmap lutBitmap) {
        nativePrepareLUT(mVulkanHandle, lutBitmap);
    }

    @Override
    public void prepareLUTs(@NonNull Bitmap[] lutArray) {
        nativePrepareLUTs(mVulkanHandle, lutArray);
    }

    @Override
    public void updateLUTs(int lutWidth, int startIndex, int drawCount, int offset) {
        nativeUpdateLUTs(mVulkanHandle, lutWidth, startIndex, drawCount, offset);
    }

    @Override
    public void updateSelectedIndex(int selectedIndex) {
        nativeUpdateSelectedIndex(mVulkanHandle, selectedIndex);
    }

    @Override
    public void prepareLongExposure(@NonNull ByteBuffer yBuffer, @NonNull ByteBuffer uBuffer, @NonNull ByteBuffer vBuffer, int w, int h, int strideY, int strideU, int strideV) {
        nativePrepareLongExposure(mVulkanHandle, yBuffer, uBuffer, vBuffer, w, h, strideY, strideU, strideV);
    }

    @Override
    public void prepare3dModel(String filePath) {
        nativePrepare3dModel(mVulkanHandle, filePath);
    }

    @Override
    public void prepare3dModelWithAnim(String filePath) {
        nativePrepare3dModelWithAnim(mVulkanHandle, filePath);
    }


    @Override
    public void prepare3dModelPBR(@NonNull String filePath) {
        nativePrepare3dModelPBR(mVulkanHandle, filePath);
    }

    @Override
    public void startRender(boolean loop) {
        if (mDrawing) {
            return;
        }

        mRenderHandler.post(() -> {
            if(loop) {
                mDrawing = true;
            }
            nativeStartRender(mVulkanHandle, loop);
        });

//        nativeStartRender(mVulkanHandle, loop);
    }

    @Override
    public void stopLoopRender() {
        nativeStopLoopRender(mVulkanHandle);

        mRenderThread.quitSafely();
        mRenderThread = null;
    }

    @Override
    public void onTouchActionMove(float deltaX, float deltaY) {
        nativeOnTouchActionMove(mVulkanHandle, deltaX, deltaY);
    }
}
