package com.example.photobooth

import android.Manifest
import android.app.Activity
import android.content.Context
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.graphics.*
import android.hardware.camera2.*
import android.media.Image
import android.media.ImageReader
import android.media.ImageReader.OnImageAvailableListener
import android.os.Environment
import android.os.Handler
import android.os.HandlerThread
import android.util.Log
import android.util.Size
import android.view.Surface
import android.view.TextureView
import android.widget.Toast
import androidx.core.content.ContextCompat
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.nio.ByteBuffer
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.Semaphore
import java.util.concurrent.TimeUnit
import kotlin.math.abs


class CameraControllerPreview {

    private var activity : Activity
    private var textureView : AutoFitTextureView


    var waitingForImage: Boolean = false
    var image : Image? = null

    private val STATE_PREVIEW = 0
    private val STATE_WAITING_LOCK = 1
    private val STATE_WAITING_PRECAPTURE = 2
    private val STATE_WAITING_NON_PRECAPTURE = 3
    private val STATE_PICTURE_TAKEN = 4
    private val MAX_PREVIEW_WIDTH = 1920
    private val MAX_PREVIEW_HEIGHT = 1080

    private var mState = STATE_PREVIEW
    private val mCameraOpenCloseLock: Semaphore = Semaphore(1)
    private var cameraDevice : CameraDevice? = null
    private var mCameraId : String = ""
    var cameraManager : CameraManager? = null
    private var imageReader: ImageReader? = null
    private var file: File? = null

    private var backgroundThread: HandlerThread? = null
    private var backgroundHandler: Handler? = null

    var captureSession : CameraCaptureSession? = null
    private var mPreviewRequestBuilder: CaptureRequest.Builder? = null
    private var mPreviewRequest: CaptureRequest? = null

    private var mFlashSupported = false
    private var mSensorOrientation: Int = 0

    private var mPreviewSize: Size = Size(0, 0)

    private inner class CameraStateCallback : CameraDevice.StateCallback() {

        override fun onOpened(camera: CameraDevice) {
            Log.d("TAG", "Camera opened successfully")
            cameraDevice = camera
            activity.runOnUiThread { Toast.makeText(activity, "Camera is on", Toast.LENGTH_SHORT).show() }
            createCameraPreviewSession();
        }

        override fun onDisconnected(camera: CameraDevice) {
            Log.d("TAG", "Camera disconnected")
            activity.runOnUiThread() { Toast.makeText(activity, "Camera is disconnected", Toast.LENGTH_SHORT).show() }
        }

        override fun onError(camera: CameraDevice, error: Int) {
            camera.close()
            cameraDevice = null
            Log.d("TAG", "Error opening camera")
        }
    }

    private val mSurfaceTextureListener: TextureView.SurfaceTextureListener = object : TextureView.SurfaceTextureListener {
        override fun onSurfaceTextureAvailable(texture: SurfaceTexture, width: Int, height: Int) {
            openCamera(width, height)
        }

        override fun onSurfaceTextureSizeChanged(texture: SurfaceTexture, width: Int, height: Int) {
            configureTransform(width, height)
        }

        override fun onSurfaceTextureDestroyed(texture: SurfaceTexture): Boolean {
            return true
        }

        override fun onSurfaceTextureUpdated(texture: SurfaceTexture) {}
    }

    internal class CompareSizesByArea : Comparator<Size?> {
        override fun compare(lhs: Size?, rhs: Size?): Int {
            // We cast here to ensure the multiplications won't overflow
            if(lhs != null && rhs != null)
            {
                return java.lang.Long.signum(lhs.width.toLong() * lhs.height - rhs.width.toLong() * rhs.height)
            }
            return 0
        }
    }

    private val mOnImageAvailableListener = OnImageAvailableListener { reader ->
        Log.d("TAG", "ImageAvailable");
        var f = file
        if(f != null)
        {
            var img = reader.acquireNextImage()
            img?.let {
                backgroundHandler?.post(ImageSaver(img, f));
            }

            image = img
        }
    }

    private val mCaptureCallback: CameraCaptureSession.CaptureCallback = object : CameraCaptureSession.CaptureCallback() {
        private fun process(result: CaptureResult) {
            when (mState) {
                STATE_WAITING_LOCK -> {
                    val afState = result[CaptureResult.CONTROL_AF_STATE]
                    if (afState == null) {
                        captureStillPicture()
                    } else if (CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED == afState ||
                            CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED == afState) {
                        // CONTROL_AE_STATE can be null on some devices
                        val aeState = result[CaptureResult.CONTROL_AE_STATE]
                        if (aeState == null || aeState == CaptureResult.CONTROL_AE_STATE_CONVERGED) {
                            mState = STATE_PICTURE_TAKEN
                            captureStillPicture()
                        } else {
                            runPrecaptureSequence()
                        }
                    }
                    Log.d("TAG", "STATE_WAITING_LOCK")
                }
                STATE_WAITING_PRECAPTURE -> {

                    // CONTROL_AE_STATE can be null on some devices
                    val aeState = result[CaptureResult.CONTROL_AE_STATE]
                    if (aeState == null || aeState == CaptureResult.CONTROL_AE_STATE_PRECAPTURE || aeState == CaptureRequest.CONTROL_AE_STATE_FLASH_REQUIRED) {
                        mState = STATE_WAITING_NON_PRECAPTURE
                    }
                    Log.d("TAG", "STATE_WAITING_PRECAPTURE")
                }
                STATE_WAITING_NON_PRECAPTURE -> {

                    // CONTROL_AE_STATE can be null on some devices
                    val aeState = result[CaptureResult.CONTROL_AE_STATE]
                    if (aeState == null || aeState != CaptureResult.CONTROL_AE_STATE_PRECAPTURE) {
                        mState = STATE_PICTURE_TAKEN
                        captureStillPicture()
                    }
                    Log.d("TAG", "STATE_WAITING_NON_PRECAPTURE")
                }
            }
        }

        override fun onCaptureProgressed(session: CameraCaptureSession, request: CaptureRequest, partialResult: CaptureResult) {
            process(partialResult)
        }

        override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
            process(result)
        }
    }

    private inner class ImageSaver(image: Image, file: File) : Runnable
    {
        private val mImage: Image
        private val mFile: File

        override fun run()
        {
            val buffer: ByteBuffer = mImage.getPlanes().get(0).getBuffer()
            val bytes = ByteArray(buffer.remaining())
            buffer.get(bytes)
            var output: FileOutputStream? = null
            try
            {
                output = FileOutputStream(mFile)
                output.write(bytes)
            } catch (e: IOException)
            {
                e.printStackTrace()
            } finally
            {
                mImage.close()
                if (output != null)
                {
                    try
                    {
                        output.close()
                    } catch (e: IOException)
                    {
                        e.printStackTrace()
                    }
                }

                waitingForImage = false
            }
        }

        init
        {
            mImage = image
            mFile = file
        }
    }

    constructor(activity: Activity, textureView: AutoFitTextureView) {
        this.activity = activity
        this.textureView = textureView
        this.textureView.setSurfaceTextureListener(mSurfaceTextureListener)
    }

    private fun getOutputMediaFile(): File?
    {
        val mediaStorageDir = File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES), "Camera2Test")

        // Create the storage directory if it does not exist
        if (!mediaStorageDir.exists() && !mediaStorageDir.mkdirs())
        {
            return null
        }

        // Create a media file name
        val timeStamp: String = SimpleDateFormat("yyyyMMdd_HHmmss").format(Date())
        return File(mediaStorageDir.path + File.separator + "IMG_" + timeStamp + ".jpg")
    }

    private fun setUpCameraOutputs(width: Int, height: Int) {
        val manager = activity.getSystemService(Context.CAMERA_SERVICE) as CameraManager
        try {
            for (cameraId in manager.cameraIdList) {
                val characteristics = manager.getCameraCharacteristics(cameraId)

                // We don't use a front facing camera in this sample.
                val facing = characteristics[CameraCharacteristics.LENS_FACING]
                if (facing != null && facing == CameraCharacteristics.LENS_FACING_FRONT) {
                    continue
                }
                val map = characteristics[CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP] ?: continue

                // For still image captures, we use the largest available size.
                var comp = CompareSizesByArea()

                var sizes : MutableList<Size> = mutableListOf<Size>()
                for(sz in map.getOutputSizes(ImageFormat.JPEG))
                {
                    Log.d("TAG", "Size avaialble : " + sz.width + "x" + sz.height)
                    if(abs((sz.width.toDouble() / sz.height) - (16.0 / 9.0)) < 0.1)
                    {
                        sizes.add(sz)
                    }
                }
                var largest: Size = sizes.maxWith(comp)!!
                //largest = Size(1920, 1080)

                imageReader = ImageReader.newInstance(largest.width, largest.height, ImageFormat.JPEG,  /*maxImages*/2)
                imageReader!!.setOnImageAvailableListener(mOnImageAvailableListener, backgroundHandler)

                // Find out if we need to swap dimension to get the preview size relative to sensor
                // coordinate.
                val displayRotation: Int = activity.getWindowManager().getDefaultDisplay().getRotation()
                val or = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION)
                if(or != null)
                {
                    mSensorOrientation = or
                }
                var swappedDimensions = false
                when (displayRotation) {
                    Surface.ROTATION_0, Surface.ROTATION_180 -> if (mSensorOrientation == 90 || mSensorOrientation == 270) {
                        swappedDimensions = true
                    }
                    Surface.ROTATION_90, Surface.ROTATION_270 -> if (mSensorOrientation == 0 || mSensorOrientation == 180) {
                        swappedDimensions = true
                    }
                    else -> Log.e("Tag", "Display rotation is invalid: $displayRotation")
                }
                val displaySize = Point()
                activity.getWindowManager().getDefaultDisplay().getSize(displaySize) //getWindowManager().getDefaultDisplay().getSize(displaySize);
                var rotatedPreviewWidth = width
                var rotatedPreviewHeight = height
                var maxPreviewWidth: Int = displaySize.x
                var maxPreviewHeight: Int = displaySize.y
                if (swappedDimensions) {
                    rotatedPreviewWidth = height
                    rotatedPreviewHeight = width
                    maxPreviewWidth = displaySize.y
                    maxPreviewHeight = displaySize.x
                }
                if (maxPreviewWidth > MAX_PREVIEW_WIDTH) {
                    maxPreviewWidth = MAX_PREVIEW_WIDTH
                }
                if (maxPreviewHeight > MAX_PREVIEW_HEIGHT) {
                    maxPreviewHeight = MAX_PREVIEW_HEIGHT
                }

                // Danger, W.R.! Attempting to use too large a preview size could  exceed the camera
                // bus' bandwidth limitation, resulting in gorgeous previews but the storage of
                // garbage capture data.
                var sz = chooseOptimalSize(map.getOutputSizes(SurfaceTexture::class.java), rotatedPreviewWidth, rotatedPreviewHeight, maxPreviewWidth, maxPreviewHeight, largest)
                sz?.let{mPreviewSize = sz}
                mPreviewSize = Size(1920, 1080)

                // We fit the aspect ratio of TextureView to the size of preview we picked.
                val orientation: Int = activity.getResources().getConfiguration().orientation
                if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
                    textureView.setAspectRatio(mPreviewSize.width, mPreviewSize.height)
                } else {
                    textureView.setAspectRatio(mPreviewSize.height, mPreviewSize.width)
                }

                // Check if the flash is supported.
                val available = characteristics[CameraCharacteristics.FLASH_INFO_AVAILABLE]
                mFlashSupported = available ?: false
                mCameraId = cameraId
                return
            }
        } catch (e: CameraAccessException) {
            e.printStackTrace()
        } catch (e: NullPointerException) {
            // Currently an NPE is thrown when the Camera2API is used but not supported on the
            // device this code runs.
            e.printStackTrace()
        }
    }

    private fun openCamera(width: Int, height: Int) {
        if (ContextCompat.checkSelfPermission(activity, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            return
        }

        setUpCameraOutputs(width, height)
        configureTransform(width, height)
        try {
            if (!mCameraOpenCloseLock.tryAcquire(2500, TimeUnit.MILLISECONDS)) {
                throw RuntimeException("Time out waiting to lock camera opening.")
            }
            startBackgroundThread()
            cameraManager = activity.getSystemService(Context.CAMERA_SERVICE) as CameraManager
            cameraManager?.openCamera(mCameraId, CameraStateCallback(), backgroundHandler)
        } catch (e: CameraAccessException) {
            e.printStackTrace()
        } catch (e: InterruptedException) {
            throw RuntimeException("Interrupted while trying to lock camera opening.", e)
        }
    }

    private fun configureTransform(viewWidth: Int, viewHeight: Int) {

        val rotation: Int = activity.getWindowManager().getDefaultDisplay().getRotation() //getWindowManager().getDefaultDisplay().getRotation();
        val matrix = Matrix()
        val viewRect = RectF(0.0f, 0.0f, viewWidth.toFloat(), viewHeight.toFloat())
        val bufferRect = RectF(0.0f, 0.0f, mPreviewSize.height.toFloat(), mPreviewSize.width.toFloat())
        val centerX: Float = viewRect.centerX()
        val centerY: Float = viewRect.centerY()
        if (Surface.ROTATION_90 == rotation || Surface.ROTATION_270 == rotation) {
            bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY())
            matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL)
            val scale = Math.max(
                    viewHeight.toFloat() / mPreviewSize.height,
                    viewWidth.toFloat() / mPreviewSize.width)
            matrix.postScale(scale, scale, centerX, centerY)
            matrix.postRotate(90.0f * (rotation - 2), centerX, centerY)
        } else if (Surface.ROTATION_180 == rotation) {
            matrix.postRotate(180.0f, centerX, centerY)
        }
        textureView?.setTransform(matrix)
    }

    private fun chooseOptimalSize(choices: Array<Size>, textureViewWidth: Int, textureViewHeight: Int, maxWidth: Int, maxHeight: Int, aspectRatio: Size): Size? {

        // Collect the supported resolutions that are at least as big as the preview Surface
        var bigEnough: MutableList<Size> = ArrayList()
        // Collect the supported resolutions that are smaller than the preview Surface
        val notBigEnough: MutableList<Size> = ArrayList()
        val w = aspectRatio.width
        val h = aspectRatio.height
        for (option in choices) {
            if (option.width <= maxWidth && option.height <= maxHeight && option.height == option.width * h / w) {
                if (option.width >= textureViewWidth &&
                        option.height >= textureViewHeight) {
                    bigEnough.add(option)
                } else {
                    notBigEnough.add(option)
                }
            }
        }

        // Pick the smallest of those big enough. If there is no one big enough, pick the
        // largest of those not big enough.
        return if (bigEnough.count() > 0) {
            Collections.min(bigEnough, CompareSizesByArea())
        } else if (notBigEnough.count() > 0) {
            Collections.max(notBigEnough, CompareSizesByArea())
        } else {
            Log.e("TAG", "Couldn't find any suitable preview size")
            choices[0]
        }
    }

    fun takePicture() : String? {
        try {
            waitingForImage = true
            file = getOutputMediaFile()
            // This is how to tell the camera to lock focus.
            mPreviewRequestBuilder?.set<Int>(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_START)
            // Tell #mCaptureCallback to wait for the lock.
            mState = STATE_WAITING_LOCK
            captureSession?.capture(mPreviewRequestBuilder!!.build(), mCaptureCallback, backgroundHandler)
        } catch (e: CameraAccessException) {
            e.printStackTrace()
        }

        activity.runOnUiThread() { Toast.makeText(activity, file?.getAbsolutePath(), Toast.LENGTH_SHORT).show() }
        return file?.path
    }

    private fun runPrecaptureSequence() {
        try {
            // This is how to tell the camera to trigger.
            mPreviewRequestBuilder!!.set<Int>(CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER, CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_START)

            // Tell #mCaptureCallback to wait for the precapture sequence to be set.
            mState = STATE_WAITING_PRECAPTURE
            captureSession?.capture(mPreviewRequestBuilder!!.build(), mCaptureCallback, backgroundHandler)
        } catch (e: CameraAccessException) {
            e.printStackTrace()
        }
    }

    private fun captureStillPicture() {
        try {
            var cam = cameraDevice
            if (null != cam) {
                // This is the CaptureRequest.Builder that we use to take a picture.
                val captureBuilder = cam.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
                captureBuilder.addTarget(imageReader!!.surface)

                // Use the same AE and AF modes as the preview.
                captureBuilder[CaptureRequest.CONTROL_AF_MODE] = CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE
                setAutoFlash(captureBuilder)

                // Orientation
                val rotation = textureView.display.rotation //getWindowManager().getDefaultDisplay().getRotation();
                captureBuilder[CaptureRequest.JPEG_ORIENTATION] = getOrientation(rotation)
                val CaptureCallback: CameraCaptureSession.CaptureCallback = object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
                        Log.d("TAG", file.toString())
                        try {
                            // Reset the auto-focus trigger
                            mPreviewRequestBuilder!!.set<Int>(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_CANCEL)
                            setAutoFlash(mPreviewRequestBuilder!!)
                            captureSession?.capture(mPreviewRequestBuilder!!.build(), mCaptureCallback, backgroundHandler)
                            // After this, the camera will go back to the normal state of preview.
                            mState = STATE_PREVIEW

                            var req = mPreviewRequest
                            if(req != null)
                            {
                                captureSession?.setRepeatingRequest(req, mCaptureCallback, backgroundHandler)
                            }
                        } catch (e: CameraAccessException) {
                            e.printStackTrace()
                        }
                    }
                }
                captureSession?.stopRepeating()
                captureSession?.capture(captureBuilder.build(), CaptureCallback, null)
            }
        } catch (e: CameraAccessException) {
            e.printStackTrace()
        }
    }

    private fun createCameraPreviewSession() {
        try {
            val texture: SurfaceTexture = textureView.getSurfaceTexture()!!

            // We configure the size of default buffer to be the size of camera preview we want.
            texture.setDefaultBufferSize(mPreviewSize.getWidth(), mPreviewSize.getHeight())

            // This is the output Surface we need to start preview.
            val surface = Surface(texture)

            // We set up a CaptureRequest.Builder with the output Surface.
            mPreviewRequestBuilder = cameraDevice?.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            mPreviewRequestBuilder?.addTarget(surface)

            // Here, we create a CameraCaptureSession for camera preview.
            cameraDevice?.createCaptureSession(Arrays.asList(surface, imageReader!!.surface),
                    object : CameraCaptureSession.StateCallback() {
                        override fun onConfigured(cameraCaptureSession: CameraCaptureSession) {
                            // The camera is already closed
                            if (null == cameraDevice) {
                                return
                            }

                            // When the session is ready, we start displaying the preview.
                            captureSession = cameraCaptureSession
                            try {
                                var builder = mPreviewRequestBuilder
                                builder?.let{
                                    // Auto focus should be continuous for camera preview.
                                    builder.set<Int>(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE)
                                    // Flash is automatically enabled when necessary.
                                    setAutoFlash(builder)

                                    // Finally, we start displaying the camera preview.
                                    var req = builder.build()
                                    mPreviewRequest = req
                                    captureSession?.setRepeatingRequest(req, mCaptureCallback, backgroundHandler)
                                }
                            } catch (e: CameraAccessException) {
                                e.printStackTrace()
                            }
                        }

                        override fun onConfigureFailed(cameraCaptureSession: CameraCaptureSession) {
                            Log.d("TAG", "Configuration Failed")
                        }
                    },
                    null)
        } catch (e: CameraAccessException) {
            e.printStackTrace()
        }
    }

    private fun getOrientation(rotation: Int): Int {
        // Sensor orientation is 90 for most devices, or 270 for some devices (eg. Nexus 5X)
        // We have to take that into account and rotate JPEG properly.
        // For devices with orientation of 90, we simply return our mapping from ORIENTATIONS.
        // For devices with orientation of 270, we need to rotate the JPEG 180 degrees.

        var rot = 0
        if(rotation == Surface.ROTATION_0)
        {
            rot = 90
        }
        else if(rotation == Surface.ROTATION_180)
        {
            rot = 270
        }
        else if(rotation == Surface.ROTATION_270)
        {
            rot = 180
        }

        return (rot + mSensorOrientation + 270) % 360
    }

    private fun setAutoFlash(requestBuilder: CaptureRequest.Builder) {
        if (mFlashSupported) {
            requestBuilder[CaptureRequest.CONTROL_AE_MODE] = CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH
        }
    }

    private fun startBackgroundThread() {
        var th =  HandlerThread("CameraBackground")
        th.start()
        backgroundHandler = Handler(th.looper)
        backgroundThread = th
    }

    private fun stopBackgroundThread() {
        backgroundThread?.quitSafely()
        try {
            backgroundThread?.join()
            backgroundThread = null
            backgroundHandler = null
        } catch (e: InterruptedException) {
            e.printStackTrace()
        }
    }
}