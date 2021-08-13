//
// Created by xiancan.wang on 8/9/21.
//
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <pthread.h>

#include "Logs.h"
#include "media/NdkMediaCodec.h"
#include "media/NdkMediaFormat.h"
#include "AACFileparser.h"
#include "JavaAudioTrackRender.h"
#include "AACMediacodecDecoder.h"

struct ThreadContextCalljava{
    JavaVM *JVM; //与进程对应的虚拟机
    jobject jobj; //对象
    char *workdir;
    char *srcfile;
    AACFileparser *pAACSrc;
    AACMediacodecDecoder *pDecoder;
    JavaAudioTrackRender *pRender;
    int sampleFrequency;
    int channel;
} gContext;

static char* jstringToChar(JNIEnv* env, jstring jstr) {
    char* rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char*) malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

void *decodeThread(void *pram){
    ThreadContextCalljava *context=(ThreadContextCalljava*)pram;
    if(context==NULL){
        ALOGD("[%s%d]context err",__FUNCTION__ ,__LINE__);
        return NULL;
    }
    int buflen =1024*100;
    unsigned char *data=(unsigned char *)malloc(buflen);
    if(data==NULL){
        ALOGD("malloc err");
        return NULL;
    }
    int framelen=0;

    int decoderStatus=0;
    while((framelen=context->pAACSrc->getOneFrameWithoutADTS(data,buflen))>0 && decoderStatus==0) {
        context->pDecoder->dodecode(data,framelen);
    }
    free(data);
    return NULL;
}

void start(JNIEnv *env, jobject obj, jstring workdir,jstring aacfile){
    gContext.jobj = env->NewGlobalRef(obj);
    gContext.workdir = jstringToChar(env,workdir);
    gContext.srcfile = jstringToChar(env,aacfile);

    int chanle=0,sampleFrequency=0,sampleFreIndex=0,profile=0;
    gContext.pAACSrc=new AACFileparser(gContext.srcfile);
    gContext.pAACSrc->probeInfo(&chanle,&sampleFrequency,&sampleFreIndex,&profile);
    gContext.sampleFrequency=sampleFrequency;
    gContext.channel=chanle;

    gContext.pRender = new JavaAudioTrackRender(gContext.JVM,gContext.jobj);
    gContext.pRender->init(gContext.sampleFrequency,gContext.channel,2);

    gContext.pDecoder = new AACMediacodecDecoder();
    gContext.pDecoder->init(gContext.channel,gContext.sampleFrequency,sampleFreIndex,profile);
    gContext.pDecoder->start(gContext.pRender);

    pthread_t pid;
    if(pthread_create(&pid,NULL,decodeThread,(void*)&gContext)<0){
        ALOGD("[%s%d] pthread create err",__FUNCTION__ ,__LINE__);
        return;
    }else{
        pthread_detach(pid);
    }
}
static JNINativeMethod gMethods[] = {
        {"start",     "(Ljava/lang/String;Ljava/lang/String;)V",      (void*)start},
};


static const char* const kClassPathName = "com/example/aacplayer/AudioTrackRender";
static int registerNativeMethods(JNIEnv* env
        , const char* className
        , JNINativeMethod* gMethods, int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

static int registerFunctios(JNIEnv *env){
    return registerNativeMethods(env,kClassPathName, gMethods, sizeof(gMethods)/sizeof(gMethods[0]));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved){
    JNIEnv* env = NULL;
    jint result = -1;
    //java　虚拟机只有一个（进程绑定），但是每一个env都是和线程绑定，所以这里如果存储env，这个env就只能在这个java的load线程中使用。
    //所以，env就不存储了，没意义
    gContext.JVM = vm;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGD("ERROR: GetEnv failed\n");
        goto bail;
    }

    if (registerFunctios(env) < 0) {
        ALOGE("[%s%d] onloader err \n",__FUNCTION__ ,__LINE__);
        goto bail;
    }
    result = JNI_VERSION_1_4;
    bail:
    return result;
}