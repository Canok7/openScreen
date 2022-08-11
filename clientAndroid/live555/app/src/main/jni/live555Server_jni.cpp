//
// Created by Administrator on 2022/8/11.
//

#define LOG_TAG "mediacode_en_jni"
#include "utils/logs.h"
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cerrno>
#include <jni.h>
#include "encoder/H264Encoder.h"
#include<android/native_window.h>
#include<android/native_window_jni.h>

class LiveServer {

public:
    ANativeWindow * getInputSurface(){
        return nullptr;
    }
    void start(){

    }
    void stop(){

    }
};

void native_start(JNIEnv *env, jobject obj, int w, int h, float fps){
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, "cObj", "J");
    auto p = (jlong) env->GetLongField(obj, fid);
    LiveServer *server = nullptr;
    if (p != 0) {
        ALOGW(" it had been create");
        server = (LiveServer *) p;
    } else {
        server = new LiveServer();
        env->SetLongField(obj, fid, (jlong) server);
    }

    server->start();
}

jobject native_getInputSurface(JNIEnv *env, jobject obj){
    auto objClazz = (jclass) env->GetObjectClass(obj);//obj为对应的JAVA对象
    jfieldID fid = env->GetFieldID(objClazz, "cObj", "J");
    auto p = (jlong) env->GetLongField(obj, fid);
    if (p == 0) {
        ALOGW("pleast call start befor stop");
        return nullptr;
    }
    auto *server = (LiveServer *) p;
    (void)server;
    // 需要转换
//    return p->getInputSurface();
return nullptr;
}

void native_stop(JNIEnv *env, jobject obj){
    auto objClazz = (jclass) env->GetObjectClass(obj);//obj为对应的JAVA对象
    jfieldID fid = env->GetFieldID(objClazz, "cObj", "J");
    auto p = (jlong) env->GetLongField(obj, fid);
    if (p == 0) {
        ALOGW("pleast call start befor stop");
        return;
    }
    auto *server = (LiveServer *) p;
    server->stop();
    delete server;
    env->SetLongField(obj, fid, 0);
}

static JNINativeMethod gMethods[] = {
        {"native_start",   "(IIF)V", (void *) native_start},
        {"native_stop",    "()V",     (void *) native_stop},
        {"native_getInputSurface", "()Landroid/view/Surface;", (void *)native_getInputSurface},
};


static const char *const kClassPathName = "com/example/live555/live555Server";

static int registerNativeMethods(JNIEnv *env, const char *className, JNINativeMethod *gMethods,
                                 int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == nullptr) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

static int registerFunctios(JNIEnv *env) {
    return registerNativeMethods(env, kClassPathName, gMethods,
                                 sizeof(gMethods) / sizeof(gMethods[0]));
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = nullptr;
    jint result = -1;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGD("ERROR: GetEnv failed\n");
        goto bail;
    }

    if (registerFunctios(env) < 0) {
        ALOGE("[%s%d] onloader err \n", __FUNCTION__, __LINE__);
        goto bail;
    }
    result = JNI_VERSION_1_4;
    bail:
    return result;
}