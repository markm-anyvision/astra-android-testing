package com.example.myapplication

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import kotlinx.android.synthetic.main.activity_main.*
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import com.orbbec.astra.*
import com.orbbec.astra.android.AstraAndroidContext
import com.orbbec.astra.android.AstraDeviceManagerListener
import java.lang.Exception
import java.util.ArrayList
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit

private const val ACTION_USB_PERMISSION = "com.android.example.USB_PERMISSION"
class MainActivity : AppCompatActivity() , AstraDeviceManagerListener {
    var rgbUsbName: String? = "jygjygj"
    var depthUsbName: String? = "jygjygj"
    lateinit var manager: UsbManager
    val devices = ArrayList<UsbDevice>()
    private var aac: AstraAndroidContext? = null
    private var executor: ExecutorService? = null

    private val runnable = Runnable {
        try {
            openDepthStream()
            /*val streamSet = StreamSet.open()
            val reader = streamSet.createReader()
            val depthStream = DepthStream.get(reader)
            depthStream.mode = ImageStreamMode(0, 640, 400, 100, 30)
            depthStream.registration = true
            val frameListener = StreamReader.FrameListener {
                    streamReader, frame ->
                val df = DepthFrame.get(frame)
                Log.i("Test", "Received frame")
            }
            reader.addFrameListener (frameListener)
            depthStream.start()

            while (!exit) {
                Astra.update()
                TimeUnit.MILLISECONDS.sleep(20L)
            }
            reader.removeFrameListener(frameListener)
            depthStream.stop()
            reader.destroy()
            streamSet.close()*/
        }
        catch (e: Exception) {

        }
    }
    /*private val usbReceiver = object : BroadcastReceiver() {

        override fun onReceive(context: Context, intent: Intent) {
            if (ACTION_USB_PERMISSION == intent.action) {
                synchronized(this) {
                    val device: UsbDevice? = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)

                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        if(devices.isNotEmpty()) {
                            val permissionIntent = PendingIntent.getBroadcast(this@MainActivity, 0, Intent(ACTION_USB_PERMISSION), 0)
                            val deviceTmp = devices[0]
                            devices.removeAt(0)
                            manager.requestPermission(deviceTmp, permissionIntent)
                        }
                        else {
                            val connection = manager.openDevice(device)
                            if(connection != null) {
                                setUsbNames(rgbUsbName!!, depthUsbName!!)
                                Log.d("Test", "All permissions approved")
                            }
                            else {

                            }
                        }
                    } else {
                        Log.d("Test", "permission denied for device $device")
                    }
                }
            }
        }
    }*/

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        requestPermissions(arrayOf(android.Manifest.permission.READ_EXTERNAL_STORAGE, android.Manifest.permission.WRITE_EXTERNAL_STORAGE), 12345)

        manager = getSystemService(Context.USB_SERVICE) as UsbManager

        // Example of a call to a native method
        sample_text.text = stringFromJNI()
        /*val filter = IntentFilter(ACTION_USB_PERMISSION)
        registerReceiver(usbReceiver, filter)
        val deviceList = manager.deviceList
        deviceList.forEach {
            if(it.value.vendorId == 11205 && it.value.productId == 1544) {
                devices.add(it.value)
                depthUsbName = it.value.deviceName
                *//*if(it.value.productName?.contains("Depth") == true) {
                    depthUsbName = it.value.deviceName

                }
                else {
                    rgbUsbName = it.value.deviceName
                }*//*
                Log.i("Test", "Device id: ${it.value.deviceId}")
                Log.i("Test", "Device name: ${it.value.deviceName}")
                Log.i("Test", "Product name: ${it.value.productName}")
                Log.i("Test", "Product id: ${it.value.productId}")
            }
        }
        val permissionIntent = PendingIntent.getBroadcast(this@MainActivity, 0, Intent(ACTION_USB_PERMISSION), 0)
        if(devices.isNotEmpty()) {
            val device = devices[0]
            devices.removeAt(0)
            manager.requestPermission(device, permissionIntent)
        }*/

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
        //unregisterReceiver(usbReceiver)
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    private external fun stringFromJNI(): String
    private external fun openDepthStream()
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
