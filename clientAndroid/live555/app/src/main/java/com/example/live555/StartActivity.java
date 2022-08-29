package com.example.live555;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.Intent;
import android.media.projection.MediaProjectionManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;


import com.example.live555.Source.MediaProjectionServer;
import com.example.live555.databinding.ActivityStartBinding;

public class StartActivity extends AppCompatActivity {
    private static final String TAG = "StartActivity";
    private ActivityStartBinding binding;
    private boolean mBStart = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityStartBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        binding.serverCamera.setOnClickListener(
                (v) -> {
                    Log.d(TAG, "onCreate: start camera Server");
                    startActivity(new Intent(StartActivity.this, CameraServerActivity.class));
                }
        );

        binding.serverScreen.setOnClickListener(
                (v) -> {
                    if(mBStart){
                        return;
                    }
                    Log.d(TAG, "onCreate: start screen Server");
                    MediaProjectionManager mMediaProjectionManager = (MediaProjectionManager) getSystemService(Context.MEDIA_PROJECTION_SERVICE);
                    //申请权限
                    startActivityForResult(mMediaProjectionManager.createScreenCaptureIntent(), MEDIA_PROJECTION_CODE);
                }
        );

        binding.client.setOnClickListener(
                (v) -> {
                    Log.d(TAG, "onCreate: start client");
                    startActivity(new Intent(StartActivity.this, ClientActivity.class));
                }
        );
    }

    /**
     * 模拟HOME键返回桌面的功能
     */
    private void simulateHome() {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.addCategory(Intent.CATEGORY_HOME);
        this.startActivity(intent);
    }

    private static final int  MEDIA_PROJECTION_CODE=11;

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if(requestCode == MEDIA_PROJECTION_CODE){
            Log.d(TAG, "onActivityResult: get MediaProjection");
            if(resultCode == RESULT_OK) {
                // 获得权限，启动Service开始录制
                Intent service = new Intent(this, MediaProjectionServer.class);
                service.putExtra("code", resultCode);
                service.putExtra("data", data);
                startService(service);
                Log.i(TAG, "Started screen recording");
                simulateHome();
                mBStart = true;
            } else {
                Toast.makeText(this, "获取权限失败", Toast.LENGTH_LONG).show();
                Log.i(TAG, "User cancelled");
            }
        }
    }

}