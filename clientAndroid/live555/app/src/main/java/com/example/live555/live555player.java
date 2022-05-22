package com.example.live555;

import android.view.Surface;

public class live555player {
    static{
        System.loadLibrary("live555player");
    }
    live555player(){

    }
    public void start(String testDir, String url, Surface surface){
        c_start(testDir,url,surface);
    }
    public void stop(){
        c_stop();
    }
    private native void c_start(String testDir, String url,Surface surface);
    private native void c_stop();
    public long cObj;
}

