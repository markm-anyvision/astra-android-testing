package com.example.myapplication

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import kotlinx.android.synthetic.main.activity_main.*
import android.hardware.usb.UsbDevice
import com.orbbec.astra.StreamSet
import com.orbbec.astra.android.AstraAndroidContext
import com.orbbec.astra.android.AstraDeviceManagerListener
import java.lang.Exception
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class MainActivity : AppCompatActivity() , AstraDeviceManagerListener {
    private var aac: AstraAndroidContext? = null
    private var executor: ExecutorService? = null

    private val runnable = Runnable {
        try {
            val streamSet = StreamSet.open()
            val deviceController = streamSet.createDeviceController()
            deviceController.enableLdp(false)
            deviceController.enableLaser(false)
            openAstraStream()
        }
        catch (e: Exception) {

        }
    }
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        // Example of a call to a native method
        aac = AstraAndroidContext(applicationContext, this)
        aac?.initialize()
        aac?.openAllDevices()


    }

    override fun onStop() {
        super.onStop()
        stopDepthStream()
    }

    override fun onDestroy() {
        super.onDestroy()
        aac?.terminate()
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    private external fun openAstraStream()
    private external fun stopDepthStream()


    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }

    override fun onOpenAllDevicesCompleted(p0: MutableIterable<UsbDevice>?) {
        executor = Executors.newSingleThreadExecutor().apply {
            execute(runnable)
        }
    }

    override fun onOpenDeviceCompleted(p0: UsbDevice?, p1: Boolean) {

    }

    override fun onPermissionDenied(p0: UsbDevice?) {

    }

    override fun onNoDevice() {

    }
}
