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
package com.gain.vulkan

import android.content.res.AssetManager
import android.graphics.Bitmap
import android.hardware.HardwareBuffer
import android.view.Surface
import java.nio.ByteBuffer

interface VulkanSample {
    fun prepare()
    fun prepareBitmap(bitmap: Bitmap)

    fun prepareI420(
        imgData: ByteArray,
        w: Int,
        h: Int,
        strideY: Int,
        strideU: Int,
        strideV: Int
    )

    fun prepareI420VkConversion(
        imgData: ByteArray,
        w: Int,
        h: Int,
        strideY: Int,
        strideU: Int,
        strideV: Int
    )

    fun prepareYUV(
        yBuffer: ByteBuffer,
        uBuffer: ByteBuffer,
        vBuffer: ByteBuffer,
        w: Int,
        h: Int,
        strideY: Int,
        strideU: Int,
        strideV: Int,
        uPixelstride: Int,
        vPixelstride: Int,
        orientation: Int = 0,
    )

    fun prepareHistogram(
        yBuffer: ByteBuffer,
        uBuffer: ByteBuffer,
        vBuffer: ByteBuffer,
        w: Int,
        h: Int,
        strideY: Int,
        strideU: Int,
        strideV: Int,
        uPixelstride: Int,
        vPixelstride: Int,
        orientation: Int = 0,
    )

    fun prepareCameraTexture()

    fun prepareLUT(lutBitmap: Bitmap)

    fun prepareLUTs(lutArray: Array<Bitmap>)

    fun updateLUTs(lutWidth:Int, startIndex:Int, drawCount:Int, offset:Int)

    fun updateSelectedIndex(selectedIndex:Int)

    fun prepareLongExposure(
        yBuffer: ByteBuffer,
        uBuffer: ByteBuffer,
        vBuffer: ByteBuffer,
        w: Int,
        h: Int,
        strideY: Int,
        strideU: Int,
        strideV: Int
    )

    fun prepare3dModel(filePath: String)

    fun prepare3dModelWithAnim(filePath: String)

    fun prepare3dModelPBR(filePath: String)

    fun startRender(loop:Boolean)

    fun stopLoopRender()

    fun setWindow(surface: Surface?, width: Int, height: Int)

    fun onTouchActionMove(deltaX:Float, deltaY:Float)

    fun init(assetManager: AssetManager, sampleType: Int)

    fun unInit()
}