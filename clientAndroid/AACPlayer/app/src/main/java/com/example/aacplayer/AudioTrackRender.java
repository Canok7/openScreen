package com.example.aacplayer;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

//由native　调用c call java
public class AudioTrackRender {
    static{
        System.loadLibrary("aacDecoder");
    }
    private static final String TAG="AudioTrackRender";
    private AudioTrack mAudioTrack;
    AudioTrackRender(){
    }
    //采样率，通道，采样精度
    public int init(int sampleRateInHz, int channelConfig,int audioFormat){

        Log.d(TAG, "init:sampleRateInHz:"+sampleRateInHz+", channelConfig:"+channelConfig+", audioFormat:"+audioFormat);
        if(channelConfig == 2){
            channelConfig = AudioFormat.CHANNEL_IN_STEREO;
        }
        if(audioFormat == 2){
            audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        }
        int minBufSize= AudioTrack.getMinBufferSize(sampleRateInHz,
                channelConfig, audioFormat);
        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,
                sampleRateInHz, channelConfig, audioFormat, minBufSize, AudioTrack.MODE_STREAM);
        if(null != mAudioTrack && mAudioTrack.getState()!=AudioTrack.STATE_UNINITIALIZED){
            //启动
            mAudioTrack.play();
            return 0;
        }else {
            return -1;
        }
    }

    public void playByteArray(byte[] audioData, int offsetInBytes, int sizeInBytes){
        if(mAudioTrack!=null && mAudioTrack.getState()!=AudioTrack.STATE_UNINITIALIZED){
            Log.d(TAG, "play: "+sizeInBytes);
            mAudioTrack.write(audioData,offsetInBytes,sizeInBytes);
        }else{
            Log.d(TAG, "play: err! not init");
        }
    }

    public void playByteBuffer(ByteBuffer audioData, int sizeInBytes){
        if(mAudioTrack!=null && mAudioTrack.getState()!=AudioTrack.STATE_UNINITIALIZED){
            Log.d(TAG, "play: "+sizeInBytes);
            mAudioTrack.write(audioData,sizeInBytes,AudioTrack.WRITE_BLOCKING);
        }else{
            Log.d(TAG, "play: err! not init");
        }
    }

    public native void start(String workdir,String aacfile);

    public void jStart(String pcmfile){
        new Thread(new Runnable() {
            @Override
            public void run() {
                InputStream is = null;
                try {
                    is = new FileInputStream(pcmfile);
                } catch (IOException e) {
                    e.printStackTrace();
                }
                init(48000,2,2);
                byte[] b = new byte[2048];
                while (true) {
                    int len;
                    try {
                        if ( (len = is.read(b)) == -1)  break;
                        Log.d(TAG, "run: write");
                        playByteArray(b, 0, len);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }, "play_thread").start();
    }
}
