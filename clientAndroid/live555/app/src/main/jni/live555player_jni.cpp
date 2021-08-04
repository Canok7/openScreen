//
// Created by canok on 2021/8/1.
//

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include <jni.h>
#include<android/native_window.h>
#include<android/native_window_jni.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include "logs.h"
#include "mediacodec/Decoder.h"
#include "live555.h"

struct RenderContext{
    ANativeWindow *pWind;
    char *testDir;
    char *url;
    CQueue *dataQueue;
    Decoder *decoder;
};
RenderContext gContext={0};

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

void Start(JNIEnv *env, jobject obj,jstring testDir, jstring url, jobject jsurface){

    //there will be 3 pthreader, one in live555, double in Mediacodec
    gContext.testDir=jstringToChar(env,testDir);
    gContext.url = jstringToChar(env,url);
    gContext.pWind = ANativeWindow_fromSurface(env, jsurface);

    ALOGD("[%s%d]  testDir:%s, url:%s,",__FUNCTION__ ,__LINE__,gContext.testDir,gContext.url);

    //udp , 100ms reorder.
    live555_start(gContext.url,gContext.testDir,false,1000*100);
    gContext.dataQueue = live55_getDataQueue();
    gContext.decoder = new Decoder();
    gContext.decoder->start(gContext.pWind,gContext.dataQueue,gContext.testDir);
}

void Stop(JNIEnv *env, jobject obj){
    gContext.decoder->stop();
    live555_stop();
    delete gContext.decoder;
    ANativeWindow_release(gContext.pWind);
    free(gContext.testDir);
    free(gContext.url);
}
static JNINativeMethod gMethods[] = {
        {"c_start",     "(Ljava/lang/String;Ljava/lang/String;Landroid/view/Surface;)V",      (void*)Start},
        {"c_stop",  "()V",   (void*)Stop},
};


static const char* const kClassPathName = "com/example/live555/live555player";
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