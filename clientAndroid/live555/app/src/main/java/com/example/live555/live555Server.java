package com.example.live555;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Surface;

import com.example.live555.Source.CameraSource;
import com.example.live555.Source.SourceInterface;

import android.os.Handler;

import androidx.annotation.NonNull;


public class live555Server {
    static {
        System.loadLibrary("live555Server");
    }

    private static final String TAG = "java_live555Server";
    private int idCount = 0;
    private Handler mHander;
    private Thread mWorkThreader = new Thread(new Runnable() {

        @Override
        public void run() {
            Looper.prepare();
            mHander = new Handler(Looper.myLooper()) {
                @SuppressLint("HandlerLeak")
                @Override
                public void handleMessage(@NonNull Message msg) {
                    switch (msg.what) {
                        case 1: {
                            Log.d(TAG, "get the notify: " + idCount++);
                            if (mSource != null) {
                                Log.d(TAG, "handleMessage:  we had start --------");
                                return;
                            }
                            if (mConfig.type == SourceInterface.SOURCE_CAMERA) {
                                Log.d(TAG, "handleMessage: create camera");
                                mSource = new CameraSource(mContext, mConfig.cameraId, mLockpriew);
                            }
                            Surface surface = native_getInputSurface();
                            if (surface == null) {
                                Log.e(TAG, "start: surface err!!!");
                            }
                            SourceInterface.SourceConfig sourc_cfg = new SourceInterface.SourceConfig();
                            sourc_cfg.fps = mConfig.fps;
                            mSource.openSource(surface, sourc_cfg);
                        }
                        break;
                        default:
                            super.handleMessage(msg);
                    }
                }
            };
            Looper.myLooper().loop();

        }
    });

    public int start(Context context) {
        ServerConfig defaultConfig = new ServerConfig();
        defaultConfig.w = 640;
        defaultConfig.h = 480;
        defaultConfig.fps = 15;
        defaultConfig.type = SourceInterface.SOURCE_CAMERA;
        this.start(context, defaultConfig, null);
        return 1;
    }

    public int stop() {
        native_stop();
        if (mSource != null) {
            mSource.closeSource();
        }
        return 1;
    }

    public int start(Context contex, ServerConfig config, Surface localprview) {
        mContext = contex;
        mConfig = config;
        mLockpriew = localprview;
        mWorkThreader.start();
        native_start(config.w, config.h, config.fps, config.workdir);
        return 1;
    }

    public void notify(int msg, int data) {
        Log.d(TAG, "notify: " + msg);
        switch (msg) {
            case 1:
                Message jmsg = Message.obtain(mHander, 1, -1, -1, null);
                jmsg.sendToTarget();
                Log.d(TAG, "notify: ");
                break;
            default:
                break;
        }
    }

    private native void native_start(int w, int h, float fps, String workdir);

    private native Surface native_getInputSurface();

    private native void native_stop();

    public long cObj;

    public static class ServerConfig {
        public String workdir;
        public int w;
        public int h;
        public float fps;
        public int type;
        public int cameraId;
    }

    private SourceInterface mSource;
    private ServerConfig mConfig;
    private Context mContext;
    private Surface mLockpriew;

}
