package com.example.myapplication

import android.hardware.usb.UsbDevice
import android.os.Bundle
import android.view.SurfaceHolder
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.orbbec.astra.android.AstraAndroidContext
import com.orbbec.astra.android.AstraDeviceManagerListener
import kotlinx.android.synthetic.main.activity_main.*
import org.freedesktop.gstreamer.GStreamer
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors


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
        // Example of a call to a native method
        aac = AstraAndroidContext(applicationContext, this)
        //startGstreamer()
        aac?.initialize()
        aac?.openAllDevices()
        /*try {
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

        }*/


    }

    override fun onStop() {
        super.onStop()
        stopDepthStream()
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
            startGstreamer()
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
        holder?.let {
            nativeSurfaceInit(it.surface)
        }
    }

    override fun surfaceDestroyed(p0: SurfaceHolder?) {
        nativeSurfaceFinalize()
    }

    override fun surfaceCreated(p0: SurfaceHolder?) {

    }

}
