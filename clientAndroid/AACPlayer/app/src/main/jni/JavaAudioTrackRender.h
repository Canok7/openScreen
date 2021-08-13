//
// Created by xiancan.wang on 8/13/21.
//

#ifndef AACPLAYER_JAVAAUDIOTRACKRENDER_H
#define AACPLAYER_JAVAAUDIOTRACKRENDER_H

#include "IAudioRender.h"
#include <jni.h>
class JavaAudioTrackRender :public IAudioRender{
public:
    JavaAudioTrackRender(JavaVM *JVM, jobject obj);
    ~JavaAudioTrackRender();
    virtual int init(int sampleRateInHz, int channelConfig,int sampleDepth);
    virtual void play(unsigned char*buf,int size);
private:
    int init_callJava();
private:
    JavaVM *mjvm; //与进程对应的虚拟机
    jclass mjcla;//该对象的类，用来获取方法
    jobject mjobj;
    JNIEnv *mEnv;//与线程绑定的env
    jmethodID mj_playByteArray;
    jmethodID mj_playByteBuffer;
    jmethodID mj_init;
    int mSampleRateInHz;
    int mChannelConfig;
    int mSampleDepth;
};


#endif //AACPLAYER_JAVAAUDIOTRACKRENDER_H
