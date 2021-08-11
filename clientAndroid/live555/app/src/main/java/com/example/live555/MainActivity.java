package com.example.live555;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {
    private static final String TAG= "MainActivity";
    private static final String testH264file = "test.264";
    private static final String SHAREDPREFERENCE_URL="_url";
    private EditText mEditUrl;
    private Button mBtnPlay;
    private SurfaceView mSurfaceView;
    private Surface mSurface;
    private String ExternalFileDir;


    private live555player mPlayer;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ExternalFileDir = getExternalFilesDir(null).getAbsolutePath();
        if(!(new File(ExternalFileDir+"/"+testH264file).exists())) {
            copyAssets(this, testH264file, ExternalFileDir);
        }
        mBtnPlay=findViewById(R.id.btnPlay);
        mBtnPlay.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(mPlayer == null && mSurface!=null){
                    mPlayer = new live555player();
                    mPlayer.start(ExternalFileDir,mEditUrl.getText().toString(),mSurface);
                }

                String url = mEditUrl.getText().toString();
                SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
                SharedPreferences.Editor editor = prefs.edit();
                editor.putString(SHAREDPREFERENCE_URL,url);
                editor.commit();
            }
        });

        mEditUrl=findViewById(R.id.url);
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        mEditUrl.setText(prefs.getString(SHAREDPREFERENCE_URL,"rtsp://192.168.1.53:8554/"));

        mSurfaceView = findViewById(R.id.surfaceView);
        SurfaceHolder holder = mSurfaceView.getHolder();
        holder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                // TODO Auto-generated method stub
                Log.i("SURFACE","destroyed");
                if(mPlayer!=null){
                    mPlayer.stop();
                }
            }

            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                // TODO Auto-generated method stub
                Log.i("SURFACE","create");
                mSurface =  holder.getSurface();
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width,
                                       int height) {
                // TODO Auto-generated method stub
            }
        });
    }


    public static void copyAssets(Context context, String srcfile, String destPath) {
        File src = new File(srcfile);
        File file = new File(destPath, src.getName());
        if (file.exists()) {
            return;
        }
        try {
            FileOutputStream fos = new FileOutputStream(file);
            InputStream is = context.getAssets().open(srcfile);
            int len;
            byte[] b = new byte[2048];
            while ((len = is.read(b)) != -1) {
                fos.write(b, 0, len);
            }
            fos.close();
            is.close();
            Log.d(TAG, "copyAssets: "+destPath+",size:"+file.getUsableSpace());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}