//
// Created by Administrator on 2022/8/11.
//

#define LOG_TAG "live555serer_jni"

#include "utils/logs.h"
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cerrno>
#include <jni.h>
#include "encoder/H264Encoder.h"
#include<android/native_window.h>
#include<android/native_window_jni.h>
#include <Serverlive555.h>
#include <Stream/h264StreamFromeFile.h>
#include <CLooper.h>
#include <CHandler.h>
#include <CMessage.h>
#include <Stream/Patch.h>

static JavaVM *gJVM; //与进程对应的虚拟机
// TODO :the CLooper not work right!
//class LiveServer : public IRtspServerNotifyer, public CHandler {
class LiveServer : public IRtspServerNotifyer {

public:
    explicit LiveServer(std::string &workdir, JavaVM *jvm, jobject obj) : mjvm(jvm), mjobj(obj) {
        mWorkdir = workdir;
// 开一个looper 用于异步处理通知信息
//        mLooper = std::make_unique<CLooper>();
//        mLooper->setName("liveServer");
//        mLooper->start();
//        mLooper->registerHandler(shared_from_this());
    }

     ~LiveServer() override {
        if (mjobj != nullptr) {
            // signal erro ! why?
////            mEnv->DeleteLocalRef(mjobj);
//            mEnv->DeleteGlobalRef(mjobj);
        }
//        mLooper->stop();
    }

    ANativeWindow *getInputSurface() {
        return mEncoder->getInputSurface();
    }

    void start(const VideoEncoderConfig &config) {
        mConfig = config;
        if (mlive555 == nullptr) {
            if (mPatch == nullptr) {
                mPatch = std::make_shared<Patch>();
            }
            mlive555 = std::make_unique<Serverlive555>(mWorkdir, mPatch);
            mlive555->start(this);
        }

//        TODO: to make encoder unitil client connected
        // 似乎在live555线程notify有客户端连接才 创建编码器，会有些问题， 这里优先把编码器创建起来。
        // 等到有客户连接，才绑定source,
        if (mEncoder == nullptr) {
            mEncoder = std::make_unique<H264Encoder>(mPatch);
        }
        mEncoder->start(mConfig, mWorkdir);

    }

    void stop() {
        mlive555->stop();
        mEncoder->stop();
        mlive555.reset();
        mEncoder.reset();
    }

private:
//    void onMessageReceived(const std::shared_ptr<CMessage> &msg) override {
//        switch (msg->what()) {
//            case IRtspServerNotifyer::MSG_NEW_CLIENT: {
//                // 需要c调用java  发送handler过去。
//                // notify(mEncoder->getInputSurface());
//                // 接着通知到java层, 由java层创建相机或者virtaulDisplay
//                if (init_callJava() == OK) {
//                    mEnv->CallVoidMethod(mjobj, mj_notify, 1, 0);
//                }
//            }
//                break;
//            case IRtspServerNotifyer::MSG_DESTTORYED: {
//                if (init_callJava() == OK) {
//                    mEnv->CallVoidMethod(mjobj, mj_notify, 2, 0);
//                }
//            }
//                break;
//            default:
//                ALOGE("[%s%d]not surpport msg:%d", __FUNCTION__, __LINE__, msg->what());
//        }
//    }

private:
    void onRtspClinetDestoryed(const ClientInfo &info) override {
//
//        std::shared_ptr<CMessage> msg = std::make_shared<CMessage>(
//                IRtspServerNotifyer::MSG_DESTTORYED, shared_from_this());
//        msg->post();
    }

    void onNewClientConnected(const ClientInfo &info) override {
        ALOGD("[%s%d]", __FUNCTION__, __LINE__);

//        if (mEncoder == nullptr) {
//            mEncoder = std::make_unique<H264Encoder>();
//        }
//        mEncoder->start(mConfig, mWorkdir);

//// 异步通知
//        std::shared_ptr<CMessage> msg = std::make_shared<CMessage>(
//                IRtspServerNotifyer::MSG_NEW_CLIENT, shared_from_this());
//        msg->post();

//     TODO:  need some client info
        if (init_callJava() == OK) {
            mEnv->CallVoidMethod(mjobj, mj_notify, 1, 0);
        }
    }

    int init_callJava() {
        if (mj_notify != nullptr) {
            return OK;
        }
        if (mjvm == nullptr) {
            ALOGD("[%s%d]jvm ==NULL,return", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }

        //env
        jint status = mjvm->GetEnv((void **) &mEnv, JNI_VERSION_1_4);
        if (status < 0) {
            ALOGD("[%s%d]attachCurrentThread", __FUNCTION__, __LINE__);
            status = mjvm->AttachCurrentThread(&mEnv, nullptr);
            if (status < 0) {
                ALOGD("[%s%d]attach err,status:%d", __FUNCTION__, __LINE__, status);
                return INVALID_OPERATION;
            }
        }
        if (mEnv == nullptr) {
            ALOGD("[%s%d]erro, no init env", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }

        //class
        ALOGD("[%s%d]getObjectClass ", __FUNCTION__, __LINE__);
        mjcla = mEnv->GetObjectClass(mjobj);
        if (mjcla == nullptr) {
            ALOGD("[%s%d]erro to get Class", __FUNCTION__, __LINE__);
            return OK;
        }
        ALOGD("[%s%d]getMethodId ", __FUNCTION__, __LINE__);
        mj_notify = mEnv->GetMethodID(mjcla, "notify", "(II)V");
        if (mj_notify == nullptr) {
            ALOGD("[%s%d]erro to find init", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }
        return OK;
    }

private:
    std::unique_ptr<Serverlive555> mlive555 = nullptr;
//    std::unique_ptr<CLooper> mLooper;
    std::unique_ptr<VideoEncoderInterface> mEncoder = nullptr;
    std::string mWorkdir;
    std::shared_ptr<Patch> mPatch = nullptr;
    VideoEncoderConfig mConfig;

    JavaVM *mjvm = nullptr; // 与进程对应的虚拟机
    jclass mjcla = nullptr; //要调用的java对象所属的类，用来获取方法
    jobject mjobj = nullptr;
    jmethodID mj_notify = nullptr; // java方法id
    JNIEnv *mEnv = nullptr; // 与线程绑定的env
};

std::string jstringToString(JNIEnv *env, jstring jstr) {
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
    std::string ret(rtn);
    free(rtn);
    return ret;
}

void native_start(JNIEnv *env, jobject obj, int w, int h, float fps, jstring workdir) {

    std::string work = jstringToString(env, workdir);

    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, "cObj", "J");
    auto p = (jlong) env->GetLongField(obj, fid);
    LiveServer *server;
    if (p != 0) {
        ALOGW(" it had been create");
        server = (LiveServer *) p;
    } else {
        server = new LiveServer(work, gJVM, env->NewGlobalRef(obj));
        env->SetLongField(obj, fid, (jlong) server);
    }

    VideoEncoderConfig config = {.w=w, .h=h, .bitRate=1024 * 5 * 1000, .fps=fps};
    server->start(config);
}

jobject native_getInputSurface(JNIEnv *env, jobject obj) {
    auto objClazz = (jclass) env->GetObjectClass(obj);//obj为对应的JAVA对象
    jfieldID fid = env->GetFieldID(objClazz, "cObj", "J");
    auto p = (jlong) env->GetLongField(obj, fid);
    if (p == 0) {
        ALOGW("pleast call start befor stop");
        return nullptr;
    }
    auto *server = (LiveServer *) p;
    ANativeWindow *nativeWindow = server->getInputSurface();
    jobject jSurface = ANativeWindow_toSurface(env, nativeWindow);
    return jSurface;
}

void native_stop(JNIEnv *env, jobject obj) {
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
        {"native_start",           "(IIFLjava/lang/String;)V", (void *) native_start},
        {"native_stop",            "()V",                      (void *) native_stop},
        {"native_getInputSurface", "()Landroid/view/Surface;", (void *) native_getInputSurface},
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
    gJVM = vm;
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