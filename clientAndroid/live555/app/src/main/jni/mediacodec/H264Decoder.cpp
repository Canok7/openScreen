//
// Created by canok on 2021/8/1.
//
#define LOG_TAG "H264Decoder"

#include "logs.h"
#include "H264Decoder.h"
#include <pthread.h>
#include <sys/time.h>
#include <cstring>
#include <unistd.h>
#include "utils/timeUtils.h"

#define TEST_FROME_FILE 0
#if TEST_FROME_FILE
#include "geth264Frame.cpp"
#endif

int64_t getNowUs() {
    struct timeval tv = {0};
    gettimeofday(&tv, nullptr);
    return (int64_t) tv.tv_sec * 1000000 + (int64_t) tv.tv_usec;
}

H264Decoder::H264Decoder() : pMediaCodec(nullptr), mThreadIn(-1), mThreadOut(-1),
                             mDataQueue(nullptr), bRun(true), bCheckSps(false) {

}

H264Decoder::~H264Decoder() {
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
    stop();
    if (pMediaCodec) {
        AMediaCodec_delete(pMediaCodec);
        pMediaCodec = nullptr;
    }
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
}

static void *input(void *privdata) {
    auto *decoder = (H264Decoder *) privdata;
    decoder->inputThread();
    return nullptr;
}

static void *output(void *privdata) {
    auto *decoder = (H264Decoder *) privdata;
    decoder->outputThread();
    return nullptr;
}

int H264Decoder::start(ANativeWindow *wind, const std::shared_ptr<MediaQueue> &dataQueues, char *workdir) {

#if TEST_FROME_FILE
    char filetestfile[128]={0};
    sprintf(filetestfile,"%s/test.264",workdir);
    if( h264_init(filetestfile) !=0){
        ALOGE("[%s%d] init file err:%s",__FUNCTION__ ,__LINE__,filetestfile);
        return -1;
    }
#endif
    if (pMediaCodec != nullptr || wind == nullptr || dataQueues == nullptr) {
        ALOGE("[%s%d] pMediaCodec!=nullptr || wind == nullptr || dataQueues == nullptr!", __FUNCTION__,
              __LINE__);
        return -1;
    }

    mDataQueue = dataQueues;
    bRun = true;
    if (pMediaCodec == nullptr) {
        pMediaCodec = AMediaCodec_createDecoderByType("video/avc");//h264
        AMediaFormat *format = AMediaFormat_new();
        AMediaFormat_setString(format, "mime", "video/avc");
        AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, 1920);
        AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, 1080);
        media_status_t status = AMediaCodec_configure(pMediaCodec, format, wind, nullptr, 0);
        AMediaFormat_delete(format);
        if (status != 0) {
            ALOGD("erro config %d", status);
            return -1;
        }
    }
    AMediaCodec_start(pMediaCodec);

    //也可以通过 AMediaCodec_setAsyncNotifyCallback 设置成异步方式.
    if (pthread_create(&mThreadIn, nullptr, input, (void *) this) != 0) {
        ALOGE("[%s%d] pthread create err!!", __FUNCTION__, __LINE__);
        return -1;
    }
    if (pthread_create(&mThreadOut, nullptr, output, (void *) this) != 0) {
        ALOGE("[%s%d] pthread create err!!", __FUNCTION__, __LINE__);
        return -1;
    }

    return 0;
}

void H264Decoder::stop() {
    bRun = false;
    if (pMediaCodec != nullptr) {
        if (mThreadIn != -1) {
            pthread_join(mThreadIn, nullptr);
            mThreadIn = -1;
        }
        if (mThreadOut != -1) {
            pthread_join(mThreadOut, nullptr);
            mThreadOut = -1;
        }
        AMediaCodec_stop(pMediaCodec);
    }
}

void H264Decoder::inputThread() {
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
    while (bRun) {
        if (mDataQueue == nullptr) {
            ALOGE("[%s%d] err", __FUNCTION__, __LINE__);
        }

        if (mDataQueue->empty()) {
            // 避免空转，还是休眠一下
            usleep(1000);
            continue;
        }
        std::shared_ptr<MediaBuffer> buffer = mDataQueue->pop();
        if (nullptr == buffer) {
            continue;
        }
        //这里开始缓冲， 将非sps pps 的也计入 到缓冲中
        // 和当前系统时间进行比较，休眠到指定时间。 才开始往下
        // 休眠可能不准确，需要更高级的方式优化
        uint64_t timenow = timeUtils::getSystemUs();
        if (buffer->dts > timenow) {
            // 缓冲中
            ALOGD("[%s%d] caching! %ld, queueSize %lu",__FUNCTION__ ,__LINE__,buffer->dts - timenow,mDataQueue->size());
            usleep(buffer->dts - timenow);
        }

        if (!bCheckSps) {
            if (checkIs_spsppsNal((unsigned char *) buffer->data())) {
                bCheckSps = true;
            } else {
                ALOGW("[%s%d] drop this frame until get a sps info ", __FUNCTION__, __LINE__);
                continue;
            }
        }
        ssize_t bufidx = 0;
        do {
            bufidx = AMediaCodec_dequeueInputBuffer(pMediaCodec, 1000 * 20);
            if (bufidx >= 0) {
                size_t bufsize;
                unsigned char nal[4] = {0x00, 0x00, 0x00, 0x01};
                uint8_t *buf = AMediaCodec_getInputBuffer(pMediaCodec, bufidx, &bufsize);
                if (bufsize - sizeof(nal) < buffer->size()) {
                    ALOGE("[%s%d]something maybe erro! will drop some video data!!! please check the mediacodec or src data:bufsize:%lu,datalen:%lu",
                          __FUNCTION__, __LINE__, bufsize, buffer->size());
                }
                memcpy(buf, nal, sizeof(nal));
                memcpy(buf + sizeof(nal), buffer->data(),
                       MIN(bufsize - sizeof(nal), buffer->size()));
                uint64_t presentationTimeUs = getNowUs();
//                ALOGD("[%s%d] AMediaCodec_queueInputBuffer one", __FUNCTION__, __LINE__);
                AMediaCodec_queueInputBuffer(pMediaCodec, bufidx, 0,
                                             MIN(bufsize, buffer->size() + 4),
                                             presentationTimeUs, 0);
            }
        } while (bufidx < 0);
    }
#endif
}

void H264Decoder::outputThread() {
    while (bRun) {
        AMediaCodecBufferInfo info;
        ssize_t bufidx = 0;
        bufidx = AMediaCodec_dequeueOutputBuffer(pMediaCodec, &info, 1000 * 20);
        if (bufidx >= 0) {
            //uint8_t *buf buf = AMediaCodec_getOutputBuffer(pMediaCodec, bufidx, &bufsize);
            AMediaCodec_releaseOutputBuffer(pMediaCodec, bufidx, true);
        } else if (bufidx == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            int w = 0, h = 0;
            auto format = AMediaCodec_getOutputFormat(pMediaCodec);
            AMediaFormat_getInt32(format, "width", &w);
            AMediaFormat_getInt32(format, "height", &h);

            float fps = 0;
//            AMediaFormat_getFloat(format,AMEDIAFORMAT_KEY_MAX_FPS_TO_ENCODER,&fps);
            int32_t localColorFMT;
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT,
                                  &localColorFMT);
            ALOGD("[%s%d] AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED:%dx%d,color:%d fps:%f",
                  __FUNCTION__,
                  __LINE__, w, h, localColorFMT, fps);
        } else if (bufidx == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
        } else {
        }
    }
}

bool H264Decoder::checkIs_spsppsNal(const unsigned char *buff) {
#if 0// 00 00 00 01 xx
    uint8_t *nal =buff;
    char type = nal[4] & ((1<<5)-1);
    //sps 7, pps 8
    return (type==7||type==8);
#else//xx
    char type = buff[0] & ((1 << 5) - 1);
    //sps 7, pps 8
//    ALOGD("[%s%d] type %d", __FUNCTION__, __LINE__, type);
    return (type == 7 || type == 8);
#endif
}