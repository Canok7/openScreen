package com.example.live555;

import android.content.Context;
import android.util.Log;
import android.view.Surface;

import com.example.live555.Source.CameraSource;
import com.example.live555.Source.SourceInterface;

public class live555Server {
    static {
        System.loadLibrary("live555Server");
    }
    private static final String TAG="live555Server";

    public int start(Context context){
        ServerConfig defaultConfig = new ServerConfig();
        defaultConfig.w = 640;
        defaultConfig.h = 480;
        defaultConfig.fps = 15;
        defaultConfig.type = SourceInterface.SOURCE_CAMERA;
        this.start(context,defaultConfig,null);
        return 1;
    }
    public int stop(){
        Log.d(TAG, "stop: ");
        native_stop();
        Log.d(TAG, "stop: ");
        mSource.closeSource();
        return 1;
    }
    public int start(Context contex, ServerConfig config, Surface localprview){
        native_start(config.w,config.h, config.fps, config.workdir);
        if(config.type == SourceInterface.SOURCE_CAMERA){
            mSource = new CameraSource(contex,config.cameraId,localprview);
        }
        Surface surface =  native_getInputSurface();
        if(surface == null){
            Log.e(TAG, "start: surface err!!!");
            return -1;
        }
        SourceInterface.SourceConfig sourc_cfg = new SourceInterface.SourceConfig();
        sourc_cfg.fps  =config.fps;
        mSource.openSource(surface,sourc_cfg);
        return 1;
    }
    private native void native_start(int w, int h, float fps, String workdir);
    private native Surface native_getInputSurface();
    private native void native_stop();

    public long cObj;

    public static class ServerConfig{
        public String workdir;
        public int w;
        public int h;
        public float fps;
        public int type;
        public int cameraId;
    }
    private SourceInterface mSource;

}
