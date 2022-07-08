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
package com.gain.vulkan.placeholder

import androidx.fragment.app.Fragment
import com.gain.vulkan.ChooseFragment
import com.gain.vulkan.samples.*
import java.util.ArrayList

/**
 * Helper class for providing sample content for user interfaces created by
 * Android template wizards.
 *
 */
object PlaceholderContent {

    /**
     * An array of sample (placeholder) items.
     */
    val ROOT_ITEMS: MutableList<PlaceholderItem> = ArrayList()

    init {
        addItem(PlaceholderItem( "Triangle+Cube", ChooseFragment.newInstance(ActionType.BASE_GRAPHICS.ordinal)))
        addItem(PlaceholderItem("Texture", ChooseFragment.newInstance(ActionType.TEXTURE.ordinal)))
        addItem(PlaceholderItem("Camera preview", CameraFragment.newInstance(SampleType.CAMERA_YUV.ordinal)))
        addItem(PlaceholderItem("Camera LUT", CameraFragment.newInstance(SampleType.LUT.ordinal)))
        addItem(PlaceholderItem("Multi LUT", CameraMultiLutFragment.newInstance(SampleType.MULTI_LUT.ordinal)))
        addItem(PlaceholderItem("histogram", CameraHistogramFragment.newInstance(SampleType.HISTOGRAM.ordinal)))
        addItem(PlaceholderItem("glTF model", SampleFragment.newInstance(SampleType.LOAD_3D_MODEL.ordinal)))
        addItem(PlaceholderItem("glTF model with anim", SampleFragment.newInstance(SampleType.LOAD_3D_MODEL_WITH_ANIM.ordinal)))
        addItem(PlaceholderItem("glTF model PBR", SampleFragment.newInstance(SampleType.LOAD_3D_MODEL_PBR.ordinal)))
    }

    private fun addItem(item: PlaceholderItem) {
        ROOT_ITEMS.add(item)
    }

    /**
     * A placeholder item representing a piece of content.
     */
    data class PlaceholderItem(val content: String, val fragment: Fragment) {
        override fun toString(): String = content
    }

    public enum class ActionType(var items: List<PlaceholderItem>) {
        BASE_GRAPHICS(listOf(
            PlaceholderItem("Triangle", SampleFragment.newInstance(SampleType.TRIANGLE.ordinal)),
            PlaceholderItem("Cube", SampleFragment.newInstance(SampleType.CUBE.ordinal))
        )),
        TEXTURE(listOf(
            PlaceholderItem("Bitmap", SampleFragment.newInstance(SampleType.TEXTURE_BITMAP.ordinal)),
            PlaceholderItem("YUV(I420)", SampleFragment.newInstance(SampleType.TEXTURE_YUV.ordinal)),
            PlaceholderItem("YUV(I420)(VK_KHR_sampler_ycbcr_conversion)", SampleFragment.newInstance(SampleType.TEXTURE_YUV_VK_CONVERSION.ordinal))
        ));

        companion object {
            fun fromInt(index: Int) = ActionType.values().first { it.ordinal == index }
        }
    }

    // 与Sample.h中SampleType保持一致
    public enum class SampleType {
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
    }
}