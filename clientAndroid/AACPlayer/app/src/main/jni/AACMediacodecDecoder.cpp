//
// Created by xiancan.wang on 8/13/21.
//

#include "AACMediacodecDecoder.h"
#include "Utils.h"
#include "Logs.h"
#include <pthread.h>
#include <string.h>
#define MIN(a,b) ((a)<(b)?(a):(b))

int64_t getNowUs(){
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
}
void *renderThread(void *pram){
    AACMediacodecDecoder *decoder=(AACMediacodecDecoder*)pram;
    if(decoder==NULL){
        ALOGD("[%s%d]decoder err",__FUNCTION__ ,__LINE__);
        return NULL;
    }
    decoder->run();
    return NULL;
}
AACMediacodecDecoder::AACMediacodecDecoder():bRun(true),pMediaCodec(NULL),pRender(NULL),pidRender(-1){

}
AACMediacodecDecoder::~AACMediacodecDecoder(){

}
int AACMediacodecDecoder::init(int chanle,int sampleFrequency,int sampleFreIndex,int profile){
    pMediaCodec = AMediaCodec_createDecoderByType("audio/mp4a-latm");//h264
    int ret =0;
    AMediaFormat* audioFormat = AMediaFormat_new();
    do {
        AMediaFormat_setString(audioFormat, "mime", "audio/mp4a-latm");
        Utils::MakeAACCodecSpecificData(audioFormat, profile, sampleFreIndex, chanle);
        //用来标记AAC是否有adts头，1->有
        AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_IS_ADTS, 0);
        media_status_t status = AMediaCodec_configure(pMediaCodec, audioFormat, NULL, NULL, 0);
        if(status!=0){
            ALOGD("erro config %d",status);
            ret = -1;
            break;
        }
    }while(0);
    AMediaFormat_delete(audioFormat);
    if(ret!=0){
        AMediaCodec_delete(pMediaCodec);
        pMediaCodec = NULL;
    }
    return ret;
}
int AACMediacodecDecoder::start(IAudioRender *render){
    if(pMediaCodec==NULL){
        ALOGE("[%s%d] pMediaCodec==NULL, not init!!",__FUNCTION__ ,__LINE__);
        return -1;
    }
    pRender = render;
    bRun=true;
    AMediaCodec_start(pMediaCodec);
    if(pthread_create(&pidRender,NULL,renderThread,(void*)this)<0){
        ALOGD("[%s%d] pthread create err",__FUNCTION__ ,__LINE__);
        return -1;
    }
    return 0;
}
int AACMediacodecDecoder::stop(){
    if(pMediaCodec!=NULL){
        bRun = false;
        AMediaCodec_stop(pMediaCodec);
        AMediaCodec_delete(pMediaCodec);
        pMediaCodec = NULL;
    }
    if(pidRender!=-1){
        pthread_join(pidRender,NULL);
    }
    return 0;
}
int AACMediacodecDecoder::dodecode(unsigned char*data,int len){
    //decoder
    int decoderStatus=0;
    ssize_t bufidx = 0;
    do {
        bufidx = AMediaCodec_dequeueInputBuffer(pMediaCodec, 1000 * 20);
        if (bufidx >= 0) {
            size_t bufsize;
            uint8_t *buf = AMediaCodec_getInputBuffer(pMediaCodec, bufidx, &bufsize);
            if (bufsize  < len) {
                ALOGE("[%s%d]something maybe erro! will drop some  data!!! please check the mediacodec or src data:bufsize:%lu,datalen:%d",
                      __FUNCTION__, __LINE__, bufsize, len);
            }
            memcpy(buf, data, MIN(len,bufsize));

            uint64_t presentationTimeUs = getNowUs();
            ALOGD("[%s%d] AMediaCodec_queueInputBuffer one", __FUNCTION__, __LINE__);
            AMediaCodec_queueInputBuffer(pMediaCodec, bufidx, 0,
                                         MIN(len,bufsize),
                                         presentationTimeUs, 0);
            break;
        }else if(bufidx == AMEDIACODEC_INFO_TRY_AGAIN_LATER){
            continue;
        } else {
            ALOGE("[%s%d] decoder err:%ld ",__FUNCTION__ ,__LINE__,bufidx);
            decoderStatus =-1;
            break;
        }
    } while (1);
    return decoderStatus;
}
void AACMediacodecDecoder::run(){
    while(bRun) {
        size_t bufsize;
        AMediaCodecBufferInfo info;
        ssize_t bufidx = 0;
        bufidx = AMediaCodec_dequeueOutputBuffer(pMediaCodec, &info, 1000 * 20);
        if (bufidx >= 0) {
            uint8_t *buf  = AMediaCodec_getOutputBuffer(pMediaCodec, bufidx, &bufsize);
            ALOGD("[%s%d] info.size:%d, bufsize:%lu",__FUNCTION__ ,__LINE__,info.size,bufsize);
            if(pRender) {
                pRender->play(buf, info.size);
            }
            AMediaCodec_releaseOutputBuffer(pMediaCodec, bufidx, false);
        } else if (bufidx == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
        } else if (bufidx == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
        } else if (bufidx == AMEDIACODEC_INFO_TRY_AGAIN_LATER){
        }else{
            ALOGE("[%s%d] mediacodec err: %ld",__FUNCTION__ ,__LINE__,bufidx);
            break;
        }
    }
}


