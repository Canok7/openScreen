package com.example.live555.Source;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.BitmapFactory;
import android.hardware.display.DisplayManager;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.IBinder;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Surface;

import androidx.annotation.Nullable;

import com.example.live555.R;
import com.example.live555.live555Server;

public class MediaProjectionServer extends Service implements SourceInterface {
    private static final String TAG = "SourceMediaProjection";
    private static final String ID = "channel_1";
    private static final String NAME = "前台服务";
    private int w;
    private int h;
    private MediaProjection mMediaProjection;
    private int mResultCode;
    private Intent mResultData;
    private live555Server mServer;

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
//        TODO: to get screen info
        w = 576;
        h = 1200;
        super.onCreate();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "onStartCommand: ");
        NotificationManager manager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        NotificationChannel channel = new NotificationChannel(ID, NAME, NotificationManager.IMPORTANCE_HIGH);
        manager.createNotificationChannel(channel);
        Notification notification = new Notification.Builder(this, ID)
                .setContentTitle("录屏")
                .setContentText("正在捕获屏幕")
                .setSmallIcon(R.mipmap.ic_launcher)
                .setLargeIcon(BitmapFactory.decodeResource(getResources(), R.mipmap.ic_launcher))
                .build();

        Log.d(TAG, "onStartCommand: set Foreground");
        /*
        startForeground执行之后，该service就是前台service了
        第一个参数表示每个通知的id
        第二个参数为要展示的通知
         */
        startForeground(1, notification);

        mResultCode = intent.getIntExtra("code", -1);
        mResultData = intent.getParcelableExtra("data");

        mMediaProjection = ((MediaProjectionManager) getSystemService(Context.MEDIA_PROJECTION_SERVICE)).getMediaProjection(mResultCode, mResultData);
        if (mMediaProjection == null) {
            Log.d(TAG, "onStartCommand: erro to get MediaProjection");
        }

        // 开始 live555 服务
        mServer = new live555Server();
        live555Server.ServerConfig config = new live555Server.ServerConfig();
        config.w = w;
        config.h = h;
        config.fps = 25;
        config.type = SourceInterface.SOURCE_SCREEN;
        config.workdir = getExternalFilesDir(null).getAbsolutePath();
        Log.d(TAG, "onStartCommand to start Server ");
        mServer.start(getApplicationContext(), config, null, this);
        return super.onStartCommand(intent, flags, startId);
    }


    private void createVirtualDisplay(Surface surface) {
//        Log.d(TAG, "create virtualDisplay: " + w + ", " + h);
        if (mMediaProjection == null) {
            mMediaProjection = ((MediaProjectionManager) getSystemService(Context.MEDIA_PROJECTION_SERVICE)).getMediaProjection(mResultCode, mResultData);
            if (mMediaProjection == null) {
                Log.d(TAG, "onStartCommand: erro to get MediaProjection");
            }
        }

        if (mMediaProjection != null) {
            Log.d(TAG, "create virtualDisplay: " + w + ", " + h);
            int flags = 0;
            int secure = 0;
            flags |= DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC
                    | DisplayManager.VIRTUAL_DISPLAY_FLAG_PRESENTATION;
            if (secure == 1) {
                flags |= DisplayManager.VIRTUAL_DISPLAY_FLAG_SECURE;
                //                                            | DisplayManager.VIRTUAL_DISPLAY_FLAG_SUPPORTS_PROTECTED_BUFFERS;
            }
            int dpi;
            dpi = Math.min(w, h)
                    * DisplayMetrics.DENSITY_XHIGH / 1080;
            mMediaProjection.createVirtualDisplay("captureScreen", w, h, dpi, flags, surface, null, null);

        } else {
            Log.e(TAG, "createVirtualDisplay erro！ ");
        }
    }


    @Override
    public void openSource(Surface surface, SourceConfig config) {
        createVirtualDisplay(surface);
    }

    @Override
    public void closeSource() {

    }

    @Override
    public void shiftSource(int id) {
        Log.e(TAG, "shiftSource: not surpport");
    }

    @Override
    public SourceInfo[] getSourceInfo() {
        return new SourceInfo[0];
    }
}
