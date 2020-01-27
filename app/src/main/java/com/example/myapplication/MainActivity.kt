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
import java.util.ArrayList

private const val ACTION_USB_PERMISSION = "com.android.example.USB_PERMISSION"
class MainActivity : AppCompatActivity() {

    lateinit var manager: UsbManager
    val devices = ArrayList<UsbDevice>()
    private val usbReceiver = object : BroadcastReceiver() {

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
                            Log.d("Test", "All permissions approved")
                        }
                    } else {
                        Log.d("Test", "permission denied for device $device")
                    }
                }
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        manager = getSystemService(Context.USB_SERVICE) as UsbManager

        // Example of a call to a native method
        sample_text.text = stringFromJNI()
        val filter = IntentFilter(ACTION_USB_PERMISSION)
        registerReceiver(usbReceiver, filter)
        val deviceList = manager.deviceList
        deviceList.forEach {
            if(it.value.vendorId == 11205) {
                devices.add(it.value)
            }
        }
        val permissionIntent = PendingIntent.getBroadcast(this@MainActivity, 0, Intent(ACTION_USB_PERMISSION), 0)
        if(devices.isNotEmpty()) {
            val device = devices[0]
            devices.removeAt(0)
            manager.requestPermission(device, permissionIntent)
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String


    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}
