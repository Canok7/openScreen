package com.example.live555;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.example.live555.Source.SourceInterface;

public class CameraServerActivity extends AppCompatActivity {
    private live555Server mServer;
    private SurfaceView mSurfaceView;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camera_server);
        requestPermission(getApplicationContext());

        mSurfaceView = findViewById(R.id.surface);
        mSurfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                // TODO Auto-generated method stub
                Log.i("SURFACE", "destroyed");
                if(mServer != null){
                    mServer.stop();
                }
            }

            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                // TODO Auto-generated method stub
                mServer = new live555Server();
                live555Server.ServerConfig config = new live555Server.ServerConfig();
                config.w=640;
                config.h=480;
                config.fps=15;
                config.type= SourceInterface.SOURCE_CAMERA;
                config.workdir = getExternalFilesDir(null).getAbsolutePath();
                mServer.start(getApplicationContext(),config,holder.getSurface());
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width,
                                       int height) {
                // TODO Auto-generated method stub
            }
        });
    }

    private void requestPermission(Context context) {
        // 先判断有没有权限
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
        } else {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, 1024);
        }

    }
}
