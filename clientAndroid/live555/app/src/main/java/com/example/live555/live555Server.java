package com.example.live555;

import android.view.Surface;

public class live555Server {
    static {
        System.loadLibrary("live555Server");
    }
    private native void native_start(int w, int h, float fps);
    private native Surface native_getInputSurface();
    private native void native_stop();

    public long cObj;
}
