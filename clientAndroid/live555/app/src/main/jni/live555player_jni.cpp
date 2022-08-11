//
// Created by canok on 2021/8/1.
//

#define LOG_TAG "live555player_jni"

#include "utils/logs.h"
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
#include "decoder/AACDecoder.h"

#include "decoder/H264Decoder.h"
#include "decoder/H265Decoder.h"
#include "live555.h"

class LivePlayer : public IRtspClientNotifyer {
public:
    explicit LivePlayer(const char *workdir) {
        ALOGD(" liveplayer %d, wordkir %s", __LINE__, workdir);
        mWorkdir = strDup(workdir);
        mLive555 = std::make_unique<SrcLive555>(mWorkdir);
        mAudioDecoder = std::make_unique<AACDecoder>();
    }

    ~LivePlayer() override {
        stop();
        delete[] mWorkdir;
    }

    void start(const char *url, ANativeWindow *pWind, bool bTcp) {
        mUrl = strDup(url);
        mPwind = pWind;
        mLive555->start(mUrl, this, bTcp, 100 * 1000, 500 * 1000);
    }

    void stop() {
        mAudioDecoder->stop();
        if (mVideoDecoder != nullptr) {
            mVideoDecoder->stop();
        }
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

    void control(int cmd, const char *stream) {
        mLive555->control(static_cast<SrcLive555::SRC_CMD>(cmd), (void *) stream);
    }

    void onRtspClinetDestoryed(void *) override {
        if (mVideoDecoder != nullptr) {
            mVideoDecoder->stop();
        }
    }

    void onNewStreamReady(std::shared_ptr<MediaQueue> queue) override {
        ALOGD("[%s%d] codec:%s", __FUNCTION__, __LINE__, queue->codecName().c_str());
        if (queue->codecName() == "MPEG4-GENERIC") {
            mAudioDecoder->start(queue, mWorkdir);
        } else if (queue->codecName() == "H264") {
            if (mVideoDecoder == nullptr) {
                mVideoDecoder = std::make_unique<H264Decoder>();
            }
            mVideoDecoder->start(mPwind, queue, mWorkdir);
        } else if (queue->codecName() == "H265") {
            if (mVideoDecoder == nullptr) {
                mVideoDecoder = std::make_unique<H265Decoder>();
            }
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
    std::unique_ptr<AACDecoder> mAudioDecoder = nullptr;
    std::unique_ptr<SrcLive555> mLive555 = nullptr;
    std::unique_ptr<VideoDecoderInterface> mVideoDecoder = nullptr;
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
    const char *workdir = jstringToChar(env, testDir);
    const char *rtspUrl = jstringToChar(env, url);
    if (p != 0) {
        ALOGW(" it had been create");
        player = (LivePlayer *) p;
    } else {
        //there will be 3 pthreader, one in live555, double in Mediacodec
        player = new LivePlayer(workdir);
        env->SetLongField(obj, fid, (jlong) player);
    }
    player->start(rtspUrl, ANativeWindow_fromSurface(env, jsurface), false);
    free((void *) workdir);
    free((void *) rtspUrl);
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

void control(JNIEnv *env, jobject obj, jint cmd, jstring stream) {
    auto objClazz = (jclass) env->GetObjectClass(obj);//obj为对应的JAVA对象
    jfieldID fid = env->GetFieldID(objClazz, "cObj", "J");
    auto p = (jlong) env->GetLongField(obj, fid);
    if (p == 0) {
        ALOGW("pleast call start befor stop");
        return;
    }
    auto *player = (LivePlayer *) p;
    const char *streamname = jstringToChar(env, stream);
    player->control(cmd, streamname);
    free((void *) streamname);
}

static JNINativeMethod gMethods[] = {
        {"c_start",   "(Ljava/lang/String;Ljava/lang/String;Landroid/view/Surface;)V", (void *) Start},
        {"c_stop",    "()V",                                                           (void *) Stop},
        {"c_control", "(ILjava/lang/String;)V",                                        (void *) control},
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