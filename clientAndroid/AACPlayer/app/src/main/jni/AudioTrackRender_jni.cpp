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
#include "getAACFrame.h"
#include "Utils.h"
#define MAKE32_LEFT(d0,d1,d2,d3) ((int32_t)(d0)<<24)|((int32_t)(d1)<<16)|((int32_t)(d2)<<8)|(int32_t)(d3)
#define MIN(a,b) ((a)<(b)?(a):(b))

#define TEST_FROME_PCMFILE 0
int64_t getNowUs(){
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
}

struct ThreadContextCalljava{
    JavaVM *JVM; //与进程对应的虚拟机
    jobject jobj; //对象
    jclass jcla;//该对象的类，用来获取方法
    JNIEnv *env;//与线程绑定的env
    jmethodID j_play; //方法id
    jmethodID j_init; //方法id
    //*********
    char *workdir;
    char *srcfile;
    AMediaCodec  *pMediaCodec;
    getAACFrame *aac;
    int sampleFrequency;
    int channel;
} gContext;

static int init_callJava(ThreadContextCalljava* context){
    if(context->JVM ==NULL){
        ALOGD("[%s%d]jvm ==NULL,return",__FUNCTION__ ,__LINE__);
        return -1;
    }

    //env
    //如果　已经attach过，这里直接可以get
    jint status = context->JVM->GetEnv((void**) &context->env, JNI_VERSION_1_4);
    if (status < 0) {
        ALOGD("[%s%d]attachCurrentThread",__FUNCTION__ ,__LINE__);
        status = context->JVM->AttachCurrentThread(&context->env, NULL);
        if (status < 0) {
            ALOGD("[%s%d]attach err,status:%d",__FUNCTION__ ,__LINE__,status);
            return -1;
        }
    }
    if(context->env==NULL){
        ALOGD("[%s%d]erro, no init env",__FUNCTION__ ,__LINE__);
        return -1;
    }

    //class
    context->jcla = context->env->GetObjectClass(context->jobj);
    if(context->jcla == NULL){
        ALOGD("[%s%d]erro to get Class",__FUNCTION__ ,__LINE__);
        return -1;
    }

    //method
    context->j_init = context->env->GetMethodID(context->jcla,"init","(III)I");
    if(context->j_init == NULL){
        ALOGD("[%s%d]erro to find init",__FUNCTION__ ,__LINE__);
        return -1;
    }
    context->j_play = context->env->GetMethodID(context->jcla,"play","([BII)V");
    if(context->j_play == NULL){
        ALOGD("[%s%d]erro to find play",__FUNCTION__ ,__LINE__);
        return -1;
    }
    return 0;
}

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

void *renderThread(void *pram){
    ThreadContextCalljava *context=(ThreadContextCalljava*)pram;
    if(context==NULL){
        ALOGD("[%s%d]context err",__FUNCTION__ ,__LINE__);
        return NULL;
    }
    init_callJava(context);
    context->env->CallIntMethod(context->jobj, context->j_init,context->sampleFrequency,context->channel,2);
#if TEST_FROME_PCMFILE
    int datalen = 2048;
    char filetest[128]={0};
    sprintf(filetest,"%s/yk_48000_2_16.pcm",context->workdir);
    FILE *fp_test= fopen(filetest,"r");
    unsigned char*buf=(unsigned char*)malloc(datalen);
    while(1) {
        int ret =fread(buf, 1, datalen, fp_test);
        if(ret<datalen){
            fseek(fp_test,0,SEEK_SET);
            ret = fread(buf, 1, datalen, fp_test);
            if(ret<datalen){
                ALOGD("[%s%d] file err",__FUNCTION__ ,__LINE__);
                break;
            }
        }

        jbyteArray byteArray = context->env->NewByteArray(datalen);
        context->env->SetByteArrayRegion(byteArray, 0, datalen, (jbyte *) buf);
        context->env->CallVoidMethod(context->jobj, context->j_play, byteArray, 0, datalen);
        context->env->DeleteLocalRef(byteArray);
    }
    fclose(fp_test);
    free(buf);
#else
    while(true) {
        size_t bufsize;
        AMediaCodecBufferInfo info;
        ssize_t bufidx = 0;
        bufidx = AMediaCodec_dequeueOutputBuffer(context->pMediaCodec, &info, 1000 * 20);
        if (bufidx >= 0) {
            //mediacode 配置的时候绑定了surface,这里就无法再取出解码后的yuv数据了，只能直接渲染
            uint8_t *buf  = AMediaCodec_getOutputBuffer(context->pMediaCodec, bufidx, &bufsize);

            ALOGD("[%s%d] info.size:%d, bufsize:%d",__FUNCTION__ ,__LINE__,info.size,bufsize);

            jbyteArray byteArray = context->env->NewByteArray(info.size);
            context->env->SetByteArrayRegion( byteArray, 0, info.size,(jbyte *)buf);
            context->env->CallVoidMethod(context->jobj, context->j_play,byteArray,0,info.size);
            context->env->DeleteLocalRef(byteArray);

            AMediaCodec_releaseOutputBuffer(context->pMediaCodec, bufidx, false);

        } else if (bufidx == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            int w = 0, h = 0;
            auto format = AMediaCodec_getOutputFormat(context->pMediaCodec);
            AMediaFormat_getInt32(format, "width", &w);
            AMediaFormat_getInt32(format, "height", &h);
            int32_t localColorFMT;
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT,
                                  &localColorFMT);
            ALOGD("[%s%d] AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED:%dx%d,color:%d", __FUNCTION__,
                  __LINE__, w, h, localColorFMT);
        } else if (bufidx == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
        } else if (bufidx == AMEDIACODEC_INFO_TRY_AGAIN_LATER){
        }else{
            ALOGE("[%s%d] mediacodec err: %d",__FUNCTION__ ,__LINE__,bufidx);
            break;
        }
    }
#endif
    return NULL;
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
    while((framelen=context->aac->getOneFrameWithoutADTS(data,buflen))>0 && decoderStatus==0) {

        //decoder
        ssize_t bufidx = 0;
        do {
            bufidx = AMediaCodec_dequeueInputBuffer(context->pMediaCodec, 1000 * 20);
            if (bufidx >= 0) {
                size_t bufsize;
                uint8_t *buf = AMediaCodec_getInputBuffer(context->pMediaCodec, bufidx, &bufsize);
                if (bufsize  < framelen) {
                    ALOGE("[%s%d]something maybe erro! will drop some  data!!! please check the mediacodec or src data:bufsize:%lu,datalen:%d",
                          __FUNCTION__, __LINE__, bufsize, framelen);
                }
                memcpy(buf, data, MIN(framelen,bufsize));

                uint64_t presentationTimeUs = getNowUs();
                ALOGD("[%s%d] AMediaCodec_queueInputBuffer one", __FUNCTION__, __LINE__);
                AMediaCodec_queueInputBuffer(context->pMediaCodec, bufidx, 0,
                                             MIN(framelen,bufsize),
                                             presentationTimeUs, 0);
                break;
            }else if(bufidx == AMEDIACODEC_INFO_TRY_AGAIN_LATER){
                continue;
            } else {
                ALOGE("[%s%d] decoder err:%d ",__FUNCTION__ ,__LINE__,bufidx);
                decoderStatus =-1;
                break;
            }
        } while (1);
    }

    free(data);
    return NULL;
}
static int initDecoder(int chanle,int sampleFrequency,int sampleFreIndex,int profile){
    gContext.pMediaCodec = AMediaCodec_createDecoderByType("audio/mp4a-latm");//h264
    AMediaFormat* audioFormat = AMediaFormat_new();
    AMediaFormat_setString(audioFormat, "mime", "audio/mp4a-latm");

    Utils::MakeAACCodecSpecificData(audioFormat,profile,sampleFreIndex,chanle);
    //用来标记AAC是否有adts头，1->有
    AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_IS_ADTS, 0);

    media_status_t status = AMediaCodec_configure(gContext.pMediaCodec,audioFormat,NULL,NULL,0);
    if(status!=0){
        ALOGD("erro config %d",status);
        return -1;
    }
    AMediaCodec_start(gContext.pMediaCodec);
    return 0;
}
void start(JNIEnv *env, jobject obj, jstring workdir,jstring aacfile){
    gContext.jobj = env->NewGlobalRef(obj);
    gContext.workdir = jstringToChar(env,workdir);
    gContext.srcfile = jstringToChar(env,aacfile);

    int chanle=0,sampleFrequency=0,sampleFreIndex=0,profile=0;
    gContext.aac=new getAACFrame(gContext.srcfile);
    gContext.aac->probeInfo(&chanle,&sampleFrequency,&sampleFreIndex,&profile);
    gContext.sampleFrequency=sampleFrequency;
    gContext.channel=chanle;
    ALOGD("[%s%d] chanle:%d, sampleFrequency:%d, profile:%d",__FUNCTION__ ,__LINE__,chanle,sampleFrequency,profile);
    initDecoder(chanle,sampleFrequency,sampleFreIndex,profile);

    pthread_t pid1,pid2;
    if(pthread_create(&pid1,NULL,renderThread,(void*)&gContext)<0){
        ALOGD("[%s%d] pthread create err",__FUNCTION__ ,__LINE__);
        return;
    }else{
        pthread_detach(pid1);
    }

    if(pthread_create(&pid2,NULL,decodeThread,(void*)&gContext)<0){
        ALOGD("[%s%d] pthread create err",__FUNCTION__ ,__LINE__);
        return;
    }else{
        pthread_detach(pid2);
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