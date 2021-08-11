package com.example.aacplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    private static  final String TAG="MainActivity";
    private Button mBtnPlay;
    private AudioTrackRender mTrackRender;
    private String mExternalFilepath;
    private static final String AACFILE="test.aac";
    private static final String PCMFILE ="yk_48000_2_16.pcm";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


        mExternalFilepath = getExternalFilesDir(null).getAbsolutePath();
        if(!(new File(mExternalFilepath+"/"+AACFILE).exists())) {
            copyAssets(this, AACFILE, mExternalFilepath);
        }
// the file too big
//        if(!(new File(mExternalFilepath+"/"+PCMFILE).exists())) {
//            copyAssets(this, PCMFILE, mExternalFilepath);
//        }
        mBtnPlay=findViewById(R.id.play);
        mBtnPlay.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(mTrackRender == null){
                    mTrackRender = new AudioTrackRender();
                    mTrackRender.start(mExternalFilepath,mExternalFilepath+"/"+AACFILE);
                    //mTrackRender.jStart(mExternalFilepath+"/"+PCMFILE);
                }
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