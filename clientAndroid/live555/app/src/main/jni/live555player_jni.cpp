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
#include <strDup.hh>
#include "logs.h"
#include "mediacodec/Decoder.h"
#include "live555.h"

class LivePlayer {
public:
    LivePlayer(const char *workdir) {
        ALOGD(" liveplayer %d, wordkir %s", __LINE__, workdir);
        mWorkdir = strDup(workdir);
        mLive555 = new SrcLive555(mWorkdir);
        mDecoder = new Decoder();
    }

    ~LivePlayer() {
        stop();
        if (mWorkdir) {
            delete[] mWorkdir;
        }
        if(mDecoder){
            delete mDecoder;
            mDecoder = nullptr;
        }
        if(mLive555){
            delete mLive555;
            mLive555 = nullptr;
        }
    }

    void start(const char *url, ANativeWindow *pWind, bool bTcp) {
        mUrl = strDup(url);
        mPwind = pWind;
        mLive555->start(mUrl, bTcp, 100 * 1000);
        mDecoder->start(mPwind, mLive555->getDataQueue(), mWorkdir);
    }

    void stop() {
        mDecoder->stop();
        mLive555->stop();
        if (mPwind) {
            ANativeWindow_release(mPwind);
            mPwind = nullptr;
        }
        if (mUrl) {
            delete[] mUrl;
            mUrl = nullptr;
        }
    }

private:
    ANativeWindow *mPwind;
    char *mUrl = nullptr;
    char *mWorkdir = nullptr;
    Decoder *mDecoder;
    SrcLive555 *mLive555;
};

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

void Start(JNIEnv *env, jobject obj, jstring testDir, jstring url, jobject jsurface) {
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, "cObj", "J");
    jlong p = (jlong) env->GetLongField(obj, fid);
    LivePlayer *player = nullptr;
    if (p != 0) {
        ALOGW(" it had been create");
        player = (LivePlayer *) p;
    } else {
        //there will be 3 pthreader, one in live555, double in Mediacodec
        player = new LivePlayer(jstringToChar(env, testDir));
        env->SetLongField(obj, fid, (jlong) player);
    }
    player->start(jstringToChar(env, url), ANativeWindow_fromSurface(env, jsurface), false);
}

void Stop(JNIEnv *env, jobject obj) {

    jclass objClazz = (jclass) env->GetObjectClass(obj);//obj为对应的JAVA对象
    jfieldID fid = env->GetFieldID(objClazz, "cObj", "J");
    jlong p = (jlong) env->GetLongField(obj, fid);
    if (p == 0) {
        ALOGW("pleast call start befor stop");
        return;
    }
    LivePlayer *player = (LivePlayer *) p;
    player->stop();
    delete player;
    env->SetLongField(obj, fid, 0);
}
static JNINativeMethod gMethods[] = {
        {"c_start",     "(Ljava/lang/String;Ljava/lang/String;Landroid/view/Surface;)V",      (void*)Start},
        {"c_stop",  "()V",   (void*)Stop},
};


static const char *const kClassPathName = "com/example/live555/live555player";

static int registerNativeMethods(JNIEnv *env, const char *className, JNINativeMethod *gMethods,
                                 int numMethods) {
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

static int registerFunctios(JNIEnv *env) {
    return registerNativeMethods(env, kClassPathName, gMethods,
                                 sizeof(gMethods) / sizeof(gMethods[0]));
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