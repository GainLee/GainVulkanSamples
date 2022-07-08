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
package com.gain.vulkan.samples

import android.app.AlertDialog
import android.app.ProgressDialog
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.view.*
import android.widget.ProgressBar
import androidx.fragment.app.Fragment
import androidx.lifecycle.lifecycleScope
import com.gain.vulkan.NativeVulkan
import com.gain.vulkan.R
import com.gain.vulkan.placeholder.PlaceholderContent
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlin.coroutines.CoroutineContext

private const val ARG_TYPE = "type"

class SampleFragment : Fragment() {
    private var type: Int? = null

    private val mRenderThread:HandlerThread by lazy { HandlerThread("render")}
    private val mRenderHandler:Handler by lazy { Handler()}

    private lateinit var vulkan: NativeVulkan

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        arguments?.let {
            type = it.getInt(ARG_TYPE)
        }

        vulkan  = NativeVulkan()
        vulkan.init(context!!.assets, type!!)
    }

    override fun onDestroy() {
        super.onDestroy()
        vulkan.unInit()
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_vulkan_content, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        var textureView = view.findViewById<TextureView>(R.id.textureview)

        textureView.surfaceTextureListener = object :
            TextureView.SurfaceTextureListener {
            override fun onSurfaceTextureAvailable(p0: SurfaceTexture, p1: Int, p2: Int) {
                vulkan.setWindow(Surface(p0), p1, p2)
                when (type) {
                    PlaceholderContent.SampleType.TRIANGLE.ordinal -> {
                        lifecycleScope.launch(Dispatchers.IO) {
                            vulkan.prepare()
                            vulkan.startRender(false)
                        }
                    }
                    PlaceholderContent.SampleType.CUBE.ordinal -> {
                        textureView.setOnTouchListener(mTouchListener)
                        vulkan.prepare()
                        vulkan.startRender(false)
                    }
                    PlaceholderContent.SampleType.TEXTURE_BITMAP.ordinal -> {
                        lifecycleScope.launch(Dispatchers.Main) {
                            var bitmap = loadBitmap("FullSizeRender.jpg")
                            vulkan.prepareBitmap(bitmap)
                            vulkan.startRender(false)
                        }
                    }
                    PlaceholderContent.SampleType.TEXTURE_YUV.ordinal -> {
                        lifecycleScope.launch(Dispatchers.Main) {
                            var imgData = loadFile("4032x3024.y420")
                            vulkan.prepareI420(imgData, 4032, 3024, 4032, 4032/2, 4032/2)
                            vulkan.startRender(false)
                        }
                    }
                    PlaceholderContent.SampleType.TEXTURE_YUV_VK_CONVERSION.ordinal -> {
                        lifecycleScope.launch(Dispatchers.Main) {
                            var imgData = loadFile("4032x3024.y420")
                            vulkan.prepareI420VkConversion(imgData, 4032, 3024, 4032, 4032/2, 4032/2)
                            vulkan.startRender(false)
                        }
                    }
                    PlaceholderContent.SampleType.LOAD_3D_MODEL.ordinal -> {
                        textureView.setOnTouchListener(mTouchListener)

                        lifecycleScope.launch(Dispatchers.IO) {
                            coroutContext = this.coroutineContext
                            var dialog = showLoadingDialog()
                            vulkan.prepare3dModel("models/DamagedHelmet/DamagedHelmet.gltf")
                            hideLoadingDialog(dialog)
                            vulkan.startRender(true)
                        }
                    }
                    PlaceholderContent.SampleType.LOAD_3D_MODEL_WITH_ANIM.ordinal -> {
                        textureView.setOnTouchListener(mTouchListener)

                        lifecycleScope.launch(Dispatchers.IO) {
                            coroutContext = this.coroutineContext
                            var dialog = showLoadingDialog()
                            vulkan.prepare3dModelWithAnim("models/CesiumMan/CesiumMan.gltf")
                            hideLoadingDialog(dialog)
                            vulkan.startRender(true)
                        }
                    }
                    PlaceholderContent.SampleType.LOAD_3D_MODEL_PBR.ordinal -> {
                        textureView.setOnTouchListener(mTouchListener)

                        lifecycleScope.launch(Dispatchers.IO) {
                            coroutContext = this.coroutineContext
                            var dialog = showLoadingDialog()
                            vulkan.prepare3dModelPBR("models/DamagedHelmet/DamagedHelmet.gltf")
                            hideLoadingDialog(dialog)
                            vulkan.startRender(true)
                        }
                    }
                }
            }

            override fun onSurfaceTextureSizeChanged(p0: SurfaceTexture, p1: Int, p2: Int) {
            }

            override fun onSurfaceTextureDestroyed(p0: SurfaceTexture): Boolean {
                return true
            }

            override fun onSurfaceTextureUpdated(p0: SurfaceTexture) {
            }
        }
    }

    override fun onStop() {
        super.onStop()
        vulkan.stopLoopRender();
    }

    lateinit var coroutContext:CoroutineContext

    var touchPosX:Float = 0f
    var touchPosY:Float = 0f
    private val mTouchListener:View.OnTouchListener = View.OnTouchListener { v, event ->
        when(event.action) {
            MotionEvent.ACTION_DOWN->{
                touchPosX = event.x
                touchPosY = event.y
            }
            MotionEvent.ACTION_MOVE->{
                var deltaX = touchPosX - event.x
                var deltaY = touchPosY - event.y

                vulkan.onTouchActionMove(deltaX, deltaY)
            }
            MotionEvent.ACTION_UP->{}
        }
        return@OnTouchListener true
    }

    suspend fun showLoadingDialog(): AlertDialog {
        return withContext(Dispatchers.Main) {
            var dialog :AlertDialog.Builder = AlertDialog.Builder(context)
            dialog.setMessage("Loading")
            dialog.setCancelable(false)

            var res = dialog.create()
            res.show()
            res
        }
    }

    suspend fun hideLoadingDialog(dialog: AlertDialog) {
        withContext(Dispatchers.Main) {
            dialog.hide()
        }
    }

    suspend fun loadBitmap(file:String):Bitmap  {
        return withContext(Dispatchers.IO) {
            var inputStream = resources.assets.open(file)
            val bitmap: Bitmap?

            inputStream.use {
                bitmap = BitmapFactory.decodeStream(it)
            }

            bitmap!!
        }
    }

    suspend fun loadFile(file:String):ByteArray  {
        return withContext(Dispatchers.IO) {
            var inputStream = resources.assets.open(file)

            inputStream.use {
                it.readBytes()
            }
        }
    }

    companion object {

        @JvmStatic
        fun newInstance(type: Int) =
            SampleFragment().apply {
                arguments = Bundle().apply {
                    putInt(ARG_TYPE, type)
                }
            }
    }
}