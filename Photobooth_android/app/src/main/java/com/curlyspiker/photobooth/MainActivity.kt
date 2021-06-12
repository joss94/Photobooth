package com.curlyspiker.photobooth

import android.content.pm.PackageManager
import android.net.wifi.WifiInfo
import android.net.wifi.WifiManager
import android.os.Bundle
import android.text.format.Formatter
import android.util.Log
import android.view.*
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.google.android.material.button.MaterialButton
import com.sun.net.httpserver.HttpExchange
import com.sun.net.httpserver.HttpHandler
import com.sun.net.httpserver.HttpServer
import java.io.File
import java.io.FileInputStream
import java.io.IOException
import java.net.InetSocketAddress
import java.util.concurrent.Executors


class MainActivity : AppCompatActivity() {

    companion object {
        private const val REQUEST_PERMISSION_CODE: Int = 1
        private val REQUIRED_PERMISSIONS: Array<String> = arrayOf(
                android.Manifest.permission.CAMERA,
                android.Manifest.permission.WRITE_EXTERNAL_STORAGE,
                android.Manifest.permission.READ_EXTERNAL_STORAGE,
                android.Manifest.permission.ACCESS_WIFI_STATE,
                android.Manifest.permission.INTERNET
        )
    }

    var ccv2WithPreview: CameraControllerPreview? = null
    var textureView: AutoFitTextureView? = null
    var ipAddressTextView: TextView? = null

    private var mHttpServer: HttpServer? = null

    private fun startServer(port: Int) {
        try {
            var wm = getSystemService(WIFI_SERVICE) as WifiManager;
            val wifiInfo: WifiInfo = wm.getConnectionInfo()
            val ip = wifiInfo.ipAddress
            val ipAddress = Formatter.formatIpAddress(ip)

            ipAddressTextView?.setText(ipAddress)

            mHttpServer = HttpServer.create(InetSocketAddress(port), 0)
            mHttpServer!!.executor = Executors.newCachedThreadPool()
            mHttpServer!!.createContext("/", rootHandler)
            // 'this' refers to the handle method
            mHttpServer!!.createContext("/takepicture", takepictureHandler)
            mHttpServer!!.start()//start server;

            ipAddressTextView?.setTextColor(getResources().getColor(R.color.green))
        } catch (e: IOException) {
            e.printStackTrace()
        }
    }

    private fun stopServer()
    {
        ipAddressTextView?.setTextColor(getResources().getColor(R.color.red))
        mHttpServer?.stop(0)
    }

    // Handler for root endpoint
    private val rootHandler = HttpHandler { exchange ->
        run {
            // Get request method
            when (exchange!!.requestMethod) {
                "GET" -> {
                    sendResponse(exchange, "Welcome to my server, to take a picture, try URI /takepicture")
                }
            }
        }
    }

    // Handler for root endpoint
    private val takepictureHandler = HttpHandler { exchange ->
        run {
            // Get request method
            when (exchange!!.requestMethod) {
                "GET" -> {
                    try {

                        var imagePath = ccv2WithPreview?.takePicture()


                        while(ccv2WithPreview!!.waitingForImage) { }
                        var f = File(imagePath)

                        exchange.getResponseHeaders().add("Content-Type", "image/jpeg");

                        val fis = FileInputStream(f)
                        val sz : Long = f.length()

                        exchange.sendResponseHeaders(200, sz)

                        val bytes = ByteArray(sz.toInt())
                        fis.read(bytes)

                        var os = exchange.responseBody

                        os.write(bytes, 0, sz.toInt())
                        os.close()
                    }
                    catch(err: Exception)
                    {
                        Log.e("TAG", "Error sending headers: " + err.message)
                    }
                }
                else -> {}
            }
        }
    }

    private fun sendResponse(httpExchange: HttpExchange, responseText: String){
        httpExchange.sendResponseHeaders(200, responseText.length.toLong())

        val os = httpExchange.responseBody
        os.write(responseText.toByteArray())
        os.close()
    }

    private fun checkRequiredPermissions(): Boolean {
        val deniedPermissions = mutableListOf<String>()

        for (permission in MainActivity.REQUIRED_PERMISSIONS) {
            if (ContextCompat.checkSelfPermission(this, permission) == PackageManager.PERMISSION_DENIED) {
                deniedPermissions.add(permission)
            }
        }
        if (deniedPermissions.isEmpty().not()) {
            requestPermissions(deniedPermissions.toTypedArray(), REQUEST_PERMISSION_CODE)
        }

        return deniedPermissions.isEmpty()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        checkRequiredPermissions()

        ipAddressTextView = findViewById(R.id.ip_address) as TextView;
        var tex = findViewById(R.id.textureview) as AutoFitTextureView;
        textureView = tex

        ccv2WithPreview = CameraControllerPreview(this, tex)

        findViewById<MaterialButton>(R.id.getpicture).setOnClickListener {
            ccv2WithPreview?.takePicture()
        }


        startServer(5000)

        window.decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_FULLSCREEN)
        val attributes = window.attributes
        attributes.flags =
            attributes.flags or (WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN or WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS)
        window.attributes = attributes
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        // Inflate the menu; this adds items to the action bar if it is present.
        menuInflater.inflate(R.menu.menu_main, menu)
        return true
    }

    override fun onDestroy() {
        super.onDestroy()
        stopServer()
    }
}