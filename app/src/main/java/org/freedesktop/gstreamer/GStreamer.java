package org.freedesktop.gstreamer;

import android.content.Context;
import android.content.res.AssetManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class GStreamer {
    private static native void nativeInit(Context context) throws Exception;

    public static void init(Context context) throws Exception {
        nativeInit(context);
    }
}
