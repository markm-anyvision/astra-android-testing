package com.example.myapplication

import android.Manifest
import android.content.pm.PackageManager
import android.hardware.usb.UsbDevice
import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.orbbec.astra.android.AstraAndroidContext
import com.orbbec.astra.android.AstraDeviceManagerListener
import kotlinx.android.synthetic.main.activity_main.*
import org.freedesktop.gstreamer.GStreamer
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

private val REQUEST_CODE_PERMISSIONS = 10
private val REQUIRED_PERMISSIONS = arrayOf(Manifest.permission.CAMERA, Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE)
class MainActivity : AppCompatActivity() , AstraDeviceManagerListener, SurfaceHolder.Callback {
    private var native_custom_data: Long = 0 //Used by native code to store private data
    private var aac: AstraAndroidContext? = null
    private var executor: ExecutorService? = null



    private val runnable = Runnable {
        try {
            try {
                GStreamer.init(this)
            } catch (e: Exception) {
                Toast.makeText(this, e.message, Toast.LENGTH_LONG).show()
                finish()
            }
            startGstreamer()
            val sh = surfaceView.holder
            sh.addCallback(this)
            nativeInit();
        }
        catch (e: Exception) {

        }
        //openAstraStream()
    }
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        aac = AstraAndroidContext(applicationContext, this)
        aac?.initialize()
        if (allPermissionsGranted()) {
            initAstra()
        } else {
            ActivityCompat.requestPermissions(
                this, REQUIRED_PERMISSIONS, REQUEST_CODE_PERMISSIONS)
        }
    }

    private fun allPermissionsGranted() = REQUIRED_PERMISSIONS.all {
        ContextCompat.checkSelfPermission(
            baseContext, it) == PackageManager.PERMISSION_GRANTED
    }

    private fun initAstra() {
        aac?.openAllDevices()
    }

    override fun onStop() {
        super.onStop()
    }

    override fun onDestroy() {
        super.onDestroy()
        aac?.terminate()
        nativeFinalize();
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    private external fun openAstraStream()
    private external fun startGstreamer()
    private external fun stopDepthStream()

    private external fun nativeInit() // Initialize native code, build pipeline, etc
    private external fun nativeFinalize() // Destroy pipeline and shutdown native code
    private external fun nativePlay() // Set pipeline to PLAYING
    private external fun nativePause() // Set pipeline to PAUSED
    private external fun nativeSurfaceInit(surface: Any)
    private external fun nativeSurfaceFinalize()

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("gstreamer_android")
            System.loadLibrary("native-lib")
            nativeClassInit()
        }

        @JvmStatic
        external fun nativeClassInit(): Boolean
    }

    override fun onOpenAllDevicesCompleted(p0: MutableIterable<UsbDevice>?) {
        /*executor = Executors.newSingleThreadExecutor().apply {
            execute(runnable)
        }*/
        try {
            try {
                GStreamer.init(this)
            } catch (e: Exception) {
                Toast.makeText(this, e.message, Toast.LENGTH_LONG).show()
                finish()
            }
            val sh = surfaceView.holder
            sh.addCallback(this)
            nativeInit();
        }
        catch (e: Exception) {

        }
    }

    override fun onOpenDeviceCompleted(p0: UsbDevice?, p1: Boolean) {

    }

    override fun onPermissionDenied(p0: UsbDevice?) {

    }

    override fun onNoDevice() {

    }

    // Called from native code. Native code calls this once it has created its pipeline and
    // the main loop is running, so it is ready to accept commands.
    private fun onGStreamerInitialized () {
        nativePlay();
    }

    override fun surfaceChanged(holder: SurfaceHolder?, p1: Int, p2: Int, p3: Int) {
        runOnUiThread {
            holder?.let {
                nativeSurfaceInit(it.surface)
            }
        }
    }

    override fun surfaceDestroyed(p0: SurfaceHolder?) {
        runOnUiThread {
            nativeSurfaceFinalize()
        }
    }

    override fun surfaceCreated(p0: SurfaceHolder?) {

    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<String>, grantResults:
        IntArray) {
        if (requestCode == REQUEST_CODE_PERMISSIONS) {
            if (allPermissionsGranted()) {
                initAstra()
            } else {
                Toast.makeText(this,
                    "Permissions not granted by the user.",
                    Toast.LENGTH_SHORT).show()
                finish()
            }
        }
    }

    private fun onDrawBox(x1: Float, y1: Float, x2: Float, y2: Float) {
        Log.i("GStreamer", "Drawing x1 $x1")
        Log.i("GStreamer", "Drawing y1 $y1")
        Log.i("GStreamer", "Drawing x2 $x2")
        Log.i("GStreamer", "Drawing y2 $y2")
        //view.clearBox()
        //view.setBox(x1, y1, x2, y2)
    }

    private fun onClearBox() {
        Log.i("GStreamer", "Clearing bounding box")
        //view.clearBox()
    }
}
