//
// Created by canok on 2021/8/1.
//

#define LOG_TAG "live555player_jni"

#include "logs.h"
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cerrno>
#include <jni.h>
#include<android/native_window.h>
#include<android/native_window_jni.h>
#include <pthread.h>
#include <unistd.h>
#include <cassert>
#include <strDup.hh>
#include <mediacodec/AACDecoder.h>

#include "mediacodec/H264Decoder.h"
#include "live555.h"

class LivePlayer : public IRtspClientNotifyer {
public:
    explicit LivePlayer(const char *workdir) {
        ALOGD(" liveplayer %d, wordkir %s", __LINE__, workdir);
        mWorkdir = strDup(workdir);
        mLive555 = std::make_unique<SrcLive555>(mWorkdir);
        mVideoDecoder = std::make_unique<H264Decoder>();
        mAudioDecoder = std::make_unique<AACDecoder>();
    }

    ~LivePlayer() override {
        stop();
//        if (mWorkdir) {
        delete[] mWorkdir;
//        }
    }

    void start(const char *url, ANativeWindow *pWind, bool bTcp) {
        mUrl = strDup(url);
        mPwind = pWind;
        mLive555->start(mUrl, this, bTcp, 100 * 1000, 500 * 1000);
    }

    void stop() {
        mAudioDecoder->stop();
        mVideoDecoder->stop();
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

    void onRtspClinetDestoryed(void *) override {
        mVideoDecoder->stop();
    }

    void onNewStreamReady(std::shared_ptr<MediaQueue> queue) override {
        ALOGD("[%s%d] codec:%s", __FUNCTION__, __LINE__, queue->codecName().c_str());
        if (queue->codecName() == "MPEG4-GENERIC") {
//        if (!queue->codecName().compare("MPEG4-GENERIC")) {
            mAudioDecoder->start(queue, mWorkdir);
        } else if (queue->codecName() == "H264") {
//        } else if (!queue->codecName().compare("H264")) {
            mVideoDecoder->start(mPwind, queue, mWorkdir);
        } else {
            ALOGW("[%s%d] unspport format code:%s", __FUNCTION__, __LINE__,
                  queue->codecName().c_str());
        }
    }

private:
    ANativeWindow *mPwind = nullptr;
    char *mUrl = nullptr;
    char *mWorkdir = nullptr;
    std::unique_ptr<H264Decoder> mVideoDecoder = nullptr;
    std::unique_ptr<AACDecoder> mAudioDecoder = nullptr;
    std::unique_ptr<SrcLive555> mLive555 = nullptr;
};

static char *jstringToChar(JNIEnv *env, jstring jstr) {
    char *rtn = nullptr;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    auto barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char *) malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

void Start(JNIEnv *env, jobject obj, jstring testDir, jstring url, jobject jsurface) {
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, "cObj", "J");
    auto p = (jlong) env->GetLongField(obj, fid);
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

    auto objClazz = (jclass) env->GetObjectClass(obj);//obj为对应的JAVA对象
    jfieldID fid = env->GetFieldID(objClazz, "cObj", "J");
    auto p = (jlong) env->GetLongField(obj, fid);
    if (p == 0) {
        ALOGW("pleast call start befor stop");
        return;
    }
    auto *player = (LivePlayer *) p;
    player->stop();
    delete player;
    env->SetLongField(obj, fid, 0);
}

static JNINativeMethod gMethods[] = {
        {"c_start", "(Ljava/lang/String;Ljava/lang/String;Landroid/view/Surface;)V", (void *) Start},
        {"c_stop",  "()V",                                                           (void *) Stop},
};


static const char *const kClassPathName = "com/example/live555/live555player";

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