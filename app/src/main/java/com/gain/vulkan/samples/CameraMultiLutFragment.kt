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

import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.*
import android.hardware.camera2.*
import android.media.Image
import android.media.ImageReader
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.util.Log
import android.util.Range
import android.view.*
import android.widget.HorizontalScrollView
import android.widget.LinearLayout
import android.widget.RadioGroup
import android.widget.Toast
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import androidx.lifecycle.lifecycleScope
import com.gain.vulkan.camera.CameraTextureView
import com.gain.vulkan.NativeVulkan
import com.gain.vulkan.R
import com.gain.vulkan.camera.CameraCore
import com.gain.vulkan.camera.getPreviewOutputSize
import com.gain.vulkan.placeholder.PlaceholderContent
import com.gain.vulkan.views.EmptyView
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.withContext
import java.io.Closeable
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.nio.ByteBuffer
import java.text.SimpleDateFormat
import java.util.*
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlin.coroutines.suspendCoroutine
import kotlin.math.ceil

private const val ARG_TYPE = "type"

private const val PERMISSIONS_REQUEST_CODE = 10

class CameraMultiLutFragment : Fragment() {
    private var type: Int? = null

    private lateinit var vulkan: NativeVulkan

    /** Where the camera preview is displayed */
    private lateinit var mPreviewView: CameraTextureView

    private lateinit var mLutContainer: RadioGroup

    private lateinit var mScrollView: HorizontalScrollView

    private var mScrollWidth: Int = -1
    private var mContainerWidth: Int = -1
    private var mItemWidth: Int = -1
    private var mDrawCount: Int = -1

    private var mLayoutChanged = false

    private var mLutConfiged = false

    /** Overlay on top of the camera preview */
    private lateinit var overlay: View

    private lateinit var mCameraCore: CameraCore

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        arguments?.let {
            type = it.getInt(ARG_TYPE)
        }

        mCameraCore = CameraCore(requireContext())
        mCameraCore.init()

        vulkan = NativeVulkan()
        vulkan.init(activity!!.assets, type!!)

        if (ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.CAMERA)
            == PackageManager.PERMISSION_DENIED) {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), PERMISSIONS_REQUEST_CODE)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == PERMISSIONS_REQUEST_CODE) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // Takes the user to the success fragment when permission is granted
                lifecycleScope.launch {
                    initializeCamera()
                }
            } else {
                Toast.makeText(context, "Permission request denied", Toast.LENGTH_LONG).show()
            }
        }
    }

    suspend fun initializeCamera() {
        mCameraCore.initializeCamera(mPreviewView.display,
            //configPreview
            { previewSize ->
                mPreviewView.setAspectRatio(previewSize)

                for (i in 0 until mLutContainer.childCount) {
                    (mLutContainer.getChildAt(i) as EmptyView).setAspectRatio(
                        previewSize
                    )
                }

                mLutContainer.requestLayout()
                mLayoutChanged = true
            },

            //onImageAvailable
            onImageAvailable@{ imageReader ->
                var yuvImage: Image? = imageReader.acquireLatestImage() ?: return@onImageAvailable

                if(mLutConfiged) {
                    val width = yuvImage!!.width
                    val height = yuvImage!!.height

                    vulkan.prepareYUV(
                        yuvImage!!.planes[0].buffer,
                        yuvImage!!.planes[1].buffer,
                        yuvImage!!.planes[2].buffer,
                        width, height,
                        yuvImage.planes[0].rowStride,
                        yuvImage.planes[1].rowStride,
                        yuvImage.planes[2].rowStride,
                        yuvImage.planes[1].pixelStride,
                        yuvImage.planes[2].pixelStride,
                        mCameraCore.getOrientation()
                    )

                    if (mLutContainer.checkedRadioButtonId < 0) {
                        lifecycleScope.launch(Dispatchers.Main) {
                            mLutContainer.check(1)
                        }
                    }

                    vulkan.startRender(false)
                }

                yuvImage!!.close()
            })
    }

    override fun onCreateView(
            inflater: LayoutInflater,
            container: ViewGroup?,
            savedInstanceState: Bundle?
    ): View? = inflater.inflate(R.layout.fragment_camera_multi_lut, container, false)

    @SuppressLint("MissingPermission")
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        mPreviewView = view.findViewById(R.id.textureview)
        mLutContainer = view.findViewById(R.id.linearlayout)

        mLutContainer.setOnCheckedChangeListener { _, checkedId ->
            //TODO: Move to render thread
            mCameraCore.runInImageReaderThread { vulkan.updateSelectedIndex(checkedId - 1) }
        }

        mScrollView = view.findViewById(R.id.scrollView)

        mLutContainer.viewTreeObserver.addOnGlobalLayoutListener {
            if(mLayoutChanged) {
                mScrollWidth = mScrollView.measuredWidth
                mContainerWidth = mLutContainer.measuredWidth
                mItemWidth = mLutContainer.getChildAt(0).measuredWidth
                mDrawCount = (ceil(mScrollWidth/mItemWidth.toDouble()) +1).toInt()

                vulkan.updateLUTs(
                    lutWidth = mItemWidth,
                    startIndex = 0,
                    drawCount = mDrawCount,
                    offset = 0
                )

                mLutConfiged = true

                mLayoutChanged = false
            }
        }

        mScrollView.setOnScrollChangeListener { v, scrollX, scrollY, oldScrollX, oldScrollY ->
            if(mScrollWidth==-1) {
                mScrollWidth = mScrollView.measuredWidth
                mContainerWidth = mLutContainer.measuredWidth
                mItemWidth = mLutContainer.getChildAt(0).measuredWidth
                mDrawCount = (ceil(mScrollWidth/mItemWidth.toDouble()) +1).toInt()
            }

            vulkan.updateLUTs(
                lutWidth = mItemWidth,
                startIndex = scrollX / mItemWidth,
                drawCount = mDrawCount,
                offset = scrollX % mItemWidth
            )
        }

        mPreviewView.surfaceTextureListener = object : TextureView.SurfaceTextureListener{
            override fun onSurfaceTextureAvailable(
                surface: SurfaceTexture,
                width: Int,
                height: Int
            ) {
                vulkan.setWindow(Surface(surface), width, height)
                lifecycleScope.launch {
                    var lutArray = arrayListOf<Bitmap>()
                    for (i in 1..9) {
                        lutArray.add(loadBitmapFromAssets("lut/lut_0$i.png"))
                    }

                    vulkan.prepareLUTs(lutArray.toTypedArray())

                    if (ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.CAMERA)
                        == PackageManager.PERMISSION_GRANTED) {
                        initializeCamera()
                    }
                }
            }

            override fun onSurfaceTextureSizeChanged(
                surface: SurfaceTexture,
                width: Int,
                height: Int
            ) {
                Log.w("Vulkan", "onSurfaceTextureSizeChanged")
            }

            override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
                Log.w("Vulkan", "onSurfaceTextureDestroyed")
                return false
            }

            override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {
            }
        }
    }

    override fun onStop() {
        super.onStop()
        try {
            mCameraCore.close()
        } catch (exc: Throwable) {
            Log.e(TAG, "Error closing camera", exc)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        vulkan.unInit()

        mCameraCore.unInit()
    }

    suspend fun loadBitmapFromAssets(filePath:String) :Bitmap {
        return withContext(Dispatchers.IO) {
            var inputStream = resources.assets.open(filePath)
            val bitmap: Bitmap?

            inputStream.use {
                bitmap = BitmapFactory.decodeStream(it)
            }

            bitmap!!
        }
    }

    suspend fun saveImage(imageBitmap: Bitmap, file: File) {
        withContext(Dispatchers.IO) {
            var output: FileOutputStream? = null
            try {
                output = FileOutputStream(file)
                imageBitmap.compress(Bitmap.CompressFormat.JPEG, 100, output)
            } catch (e: IOException) {
                e.printStackTrace()
                Log.e("GLSL", "saveImage error", e)
            } finally {
                if (null != output) {
                    try {
                        output.close()
                    } catch (e: IOException) {
                        e.printStackTrace()
                    }
                }
            }
        }
    }

    suspend fun saveBytes(data: ByteArray, file: File) {
        withContext(Dispatchers.IO) {
            var output: FileOutputStream? = null
            try {
                output = FileOutputStream(file)
                output.write(data, 0, data.size)
                output.flush()
            } catch (e: IOException) {
                e.printStackTrace()
                Log.e("GLSL", "saveBytes error", e)
            } finally {
                if (null != output) {
                    try {
                        output.close()
                    } catch (e: IOException) {
                        e.printStackTrace()
                    }
                }
            }
        }
    }

    companion object {
        private val TAG = CameraMultiLutFragment::class.java.simpleName

        /** Helper data class used to hold capture metadata with their associated image */
        data class CombinedCaptureResult(
                val image: Image,
                val metadata: CaptureResult,
                val orientation: Int,
                val format: Int
        ) : Closeable {
            override fun close() = image.close()
        }

        /**
         * Create a [File] named a using formatted timestamp with the current date and time.
         *
         * @return [File] created.
         */
        private fun createFile(context: Context, extension: String): File {
            val sdf = SimpleDateFormat("yyyy_MM_dd_HH_mm_ss_SSS", Locale.US)
            return File(context.filesDir, "IMG_${sdf.format(Date())}.$extension")
        }

        fun getOutputDirectory(context: Context): File {
            val appContext = context.applicationContext
            val mediaDir = context.externalMediaDirs.firstOrNull()?.let {
                File(it, appContext.resources.getString(R.string.app_name)).apply { mkdirs() }
            }
            return if (mediaDir != null && mediaDir.exists())
                mediaDir else appContext.filesDir
        }

        @JvmStatic
        fun newInstance(type: Int) =
            CameraMultiLutFragment().apply {
                arguments = Bundle().apply {
                    putInt(ARG_TYPE, type)
                }
            }
    }
}
