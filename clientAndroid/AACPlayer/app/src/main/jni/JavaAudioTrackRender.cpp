//
// Created by xiancan.wang on 8/13/21.
//

#include "JavaAudioTrackRender.h"
#include "Logs.h"
JavaAudioTrackRender::JavaAudioTrackRender(JavaVM *JVM, jobject obj):
        mjvm(JVM),mjcla(NULL),mjobj(obj),
        mEnv(NULL),mj_playByteBuffer(NULL),mj_playByteArray(NULL),mj_init(NULL),
        mSampleRateInHz(0),mChannelConfig(0),mSampleDepth(0)
{

}
JavaAudioTrackRender::~JavaAudioTrackRender(){

}

int JavaAudioTrackRender::init(int sampleRateInHz, int channelConfig,int sampleDepth){

//    init_callJava();
//    mEnv->CallIntMethod(mjobj, mj_init,sampleRateInHz,channelConfig,sampleDepth);
//    return 0;
//most called on one thread which call play
    mSampleRateInHz = sampleRateInHz;
    mChannelConfig = channelConfig;
    mSampleDepth = sampleDepth;
    return 0;
}
void JavaAudioTrackRender::play(unsigned char*buf,int size) {
    init_callJava();
    jbyteArray byteArray = mEnv->NewByteArray(size);
    mEnv->SetByteArrayRegion(byteArray, 0, size, (jbyte *) buf);
    mEnv->CallVoidMethod(mjobj, mj_playByteArray, byteArray, 0, size);
}
int JavaAudioTrackRender::init_callJava(){
    if(mj_init!=NULL && mj_playByteArray!=NULL && mj_playByteBuffer!=NULL){
        return 0;
    }
    if(mjvm ==NULL){
        ALOGD("[%s%d]jvm ==NULL,return",__FUNCTION__ ,__LINE__);
        return -1;
    }

    //env
    //如果　已经attach过，这里直接可以get
    jint status =mjvm->GetEnv((void**) &mEnv, JNI_VERSION_1_4);
    if (status < 0) {
        ALOGD("[%s%d]attachCurrentThread",__FUNCTION__ ,__LINE__);
        status =mjvm->AttachCurrentThread(&mEnv, NULL);
        if (status < 0) {
            ALOGD("[%s%d]attach err,status:%d",__FUNCTION__ ,__LINE__,status);
            return -1;
        }
    }
    if(mEnv==NULL){
        ALOGD("[%s%d]erro, no init env",__FUNCTION__ ,__LINE__);
        return -1;
    }

    //class
    mjcla = mEnv->GetObjectClass(mjobj);
    if(mjcla == NULL){
        ALOGD("[%s%d]erro to get Class",__FUNCTION__ ,__LINE__);
        return -1;
    }

    mj_init = mEnv->GetMethodID(mjcla,"init","(III)I");
    if(mj_init == NULL){
        ALOGD("[%s%d]erro to find init",__FUNCTION__ ,__LINE__);
        return -1;
    }
    mj_playByteArray = mEnv->GetMethodID(mjcla,"playByteArray","([BII)V");
    if(mj_playByteArray == NULL){
        ALOGD("[%s%d]erro to find play",__FUNCTION__ ,__LINE__);
        return -1;
    }

    mj_playByteBuffer = mEnv->GetMethodID(mjcla,"playByteBuffer","(Ljava/nio/ByteBuffer;I)V");
    if(mj_playByteBuffer == NULL){
        ALOGD("[%s%d]erro to find play",__FUNCTION__ ,__LINE__);
        return -1;
    }

   return mEnv->CallIntMethod(mjobj, mj_init,mSampleRateInHz,mChannelConfig,mSampleDepth);
}