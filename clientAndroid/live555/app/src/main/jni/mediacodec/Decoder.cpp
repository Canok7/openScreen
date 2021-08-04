//
// Created by canok on 2021/8/1.
//

#include "Decoder.h"
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#define TEST_FROME_FILE 0
#if TEST_FROME_FILE
#include "geth264Frame.cpp"
#else
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
int64_t getNowUs(){
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
}

Decoder::Decoder():pMediaCodec(NULL),mThreadIn(-1),mThreadOut(-1),
mDataQueue(NULL),bRun(true),bCheckSps(false){

}

Decoder::~Decoder(){

}

static void *input(void *privdata){
    Decoder *decoder = (Decoder*)privdata;
    decoder->inputThread();
    return NULL;
}
static void *output(void *privdata){
    Decoder *decoder = (Decoder*)privdata;
    decoder->outputThread();
    return NULL;
}
int Decoder::start(ANativeWindow *wind,CQueue *dataQueue, char* workdir){
#if TEST_FROME_FILE
    char filetestfile[128]={0};
    sprintf(filetestfile,"%s/test.264",workdir);
    if( h264_init(filetestfile) !=0){
        ALOGE("[%s%d] init file err:%s",__FUNCTION__ ,__LINE__,filetestfile);
        return -1;
    }
#endif
    if(pMediaCodec !=NULL){
        ALOGE("[%s%d] had start!!",__FUNCTION__ ,__LINE__);
        return -1;
    }
    mDataQueue = dataQueue;
    pMediaCodec = AMediaCodec_createDecoderByType("video/avc");//h264
    AMediaFormat *format = AMediaFormat_new();
    AMediaFormat_setString(format, "mime", "video/avc");
    AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_WIDTH,1920);
    AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_HEIGHT,1080);
    media_status_t status = AMediaCodec_configure(pMediaCodec,format,wind,NULL,0);
    if(status!=0){
        ALOGD("erro config %d",status);
        return -1;
    }
    AMediaCodec_start(pMediaCodec);

    //也可以通过 AMediaCodec_setAsyncNotifyCallback 设置成异步方式.
    if(pthread_create(&mThreadIn,NULL,input,(void*)this)<0){
        ALOGE("[%s%d] pthread create err!!",__FUNCTION__ ,__LINE__);
        return -1;
    }
    if(pthread_create(&mThreadOut,NULL,output,(void*)this)<0){
        ALOGE("[%s%d] pthread create err!!",__FUNCTION__ ,__LINE__);
        return -1;
    }
    AMediaFormat_delete(format);
    return 0;
}

void Decoder::stop(){
    if(mThreadIn!=-1){
        pthread_join(mThreadIn,NULL);
    }
    if(mThreadOut!=-1) {
        pthread_join(mThreadOut, NULL);
    }
}
void Decoder::inputThread(){
#if TEST_FROME_FILE
    ssize_t bufidx = 0;
    int buflen=1024*500;
    unsigned char*bufin = (unsigned  char*)malloc(buflen);
    int datalen =0;

    while(bRun) {
        datalen = h264_getOneNal(bufin,buflen);
        do {
            bufidx = AMediaCodec_dequeueInputBuffer(pMediaCodec, 1000 * 20);
            if (bufidx >= 0) {
                size_t bufsize;
                uint8_t *buf = AMediaCodec_getInputBuffer(pMediaCodec, bufidx, &bufsize);
                if (bufsize < datalen) {
                    ALOGE("[%s%d]something maybe erro! will drop some video data!!! please check the mediacodec or src data:bufsize:%lu,datalen:%d",
                          __FUNCTION__, __LINE__, bufsize, datalen);
                }
                memcpy(buf, bufin, MIN(bufsize, datalen));
                uint64_t presentationTimeUs = getNowUs();
                ALOGD("[%s%d] AMediaCodec_queueInputBuffer one", __FUNCTION__, __LINE__);
                AMediaCodec_queueInputBuffer(pMediaCodec, bufidx, 0, MIN(bufsize, datalen),
                                             presentationTimeUs, 0);
            }
        } while (bufidx < 0);
    }
#else
    int queueIndex=0;
    uint8_t *data=NULL;
    while(bRun){
        int datalen=0;
        do {
             //Must be released by mDataQueue->releasebuffer(queueIndex) later
             //will block until get one buffer
            queueIndex = mDataQueue->getbuffer(&data, &datalen);
            if(!bCheckSps){
                if(checkIs_spsppsNal(data)){
                    bCheckSps = true;
                }else{
                    ALOGW("[%s%d] drop this frame until get a sps info ",__FUNCTION__ ,__LINE__);
                    break;
                }
            }
            ssize_t bufidx = 0;
            do {
                bufidx = AMediaCodec_dequeueInputBuffer(pMediaCodec, 1000*20);
                if (bufidx >= 0) {
                    size_t bufsize;
                    unsigned char nal[4]={0x00,0x00,0x00,0x01};
                    uint8_t *buf = AMediaCodec_getInputBuffer(pMediaCodec, bufidx, &bufsize);
                    if (bufsize - sizeof(nal) < datalen) {
                        ALOGE("[%s%d]something maybe erro! will drop some video data!!! please check the mediacodec or src data:bufsize:%lu,datalen:%d",
                              __FUNCTION__, __LINE__, bufsize, datalen);
                    }
                    memcpy(buf,nal,sizeof(nal));
                    memcpy(buf+sizeof(nal), data, MIN(bufsize-sizeof(nal), datalen));
                    uint64_t presentationTimeUs = getNowUs();
                    ALOGD("[%s%d] AMediaCodec_queueInputBuffer one",__FUNCTION__ ,__LINE__);
                    AMediaCodec_queueInputBuffer(pMediaCodec, bufidx, 0, MIN(bufsize, datalen+4),
                                                 presentationTimeUs, 0);
                }
            }while(bufidx <0);
        }while(0);
        mDataQueue->releasebuffer(queueIndex);
    }
#endif
}

void Decoder::outputThread() {
    while (bRun) {
        size_t bufsize;
        AMediaCodecBufferInfo info;
        ssize_t bufidx = 0;
        bufidx = AMediaCodec_dequeueOutputBuffer(pMediaCodec, &info, 1000 * 20);
        if (bufidx >= 0) {
            //mediacode 配置的时候绑定了surface,这里就无法再取出解码后的yuv数据了，只能直接渲染
            //uint8_t *buf buf = AMediaCodec_getOutputBuffer(pMediaCodec, bufidx, &bufsize);
            AMediaCodec_releaseOutputBuffer(pMediaCodec, bufidx, true);
        } else if (bufidx == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            int w = 0, h = 0;
            auto format = AMediaCodec_getOutputFormat(pMediaCodec);
            AMediaFormat_getInt32(format, "width", &w);
            AMediaFormat_getInt32(format, "height", &h);
            int32_t localColorFMT;
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT,
                                  &localColorFMT);
            ALOGD("[%s%d] AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED:%dx%d,color:%d", __FUNCTION__,
                  __LINE__, w, h, localColorFMT);
        } else if (bufidx == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
        } else {
        }
    }
}

bool Decoder::checkIs_spsppsNal(unsigned char* buff){
#if 0// 00 00 00 01 xx
    uint8_t *nal =buff;
    char type = nal[4] & ((1<<5)-1);
    //sps 7, pps 8
    return (type==7||type==8);
#else//xx
    char type = buff[0] & ((1<<5)-1);
    //sps 7, pps 8
    return (type==7||type==8);
#endif
}