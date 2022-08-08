//
// Created by Administrator on 2022/8/6.
//

#define LOG_TAG "H265Decoder"

#include "logs.h"
#include "H265Decoder.h"
#include <pthread.h>
#include <sys/time.h>
#include <cstring>
#include <unistd.h>
#include "utils/timeUtils.h"

H265Decoder::H265Decoder() {

}

H265Decoder::~H265Decoder() {
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
    stop();
    if (pMediaCodec) {
        AMediaCodec_delete(pMediaCodec);
        pMediaCodec = nullptr;
    }
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
}

static void *input(void *privdata) {
    auto *decoder = (H265Decoder *) privdata;
    decoder->inputThread();
    return nullptr;
}

static void *output(void *privdata) {
    auto *decoder = (H265Decoder *) privdata;
    decoder->outputThread();
    return nullptr;
}

int H265Decoder::start(ANativeWindow *wind, const std::shared_ptr<MediaQueue> &dataQueues,
                       char *workdir) {

    if (pMediaCodec != nullptr || wind == nullptr || dataQueues == nullptr) {
        ALOGE("[%s%d] pMediaCodec!=nullptr || wind == nullptr || dataQueues == nullptr!",
              __FUNCTION__,
              __LINE__);
        return -1;
    }

    mDataQueue = dataQueues;
    bRun = true;
    if (pMediaCodec == nullptr) {
        pMediaCodec = AMediaCodec_createDecoderByType("video/hevc");//h265
        AMediaFormat *format = AMediaFormat_new();
        AMediaFormat_setString(format, "mime", "video/hevc");
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

void H265Decoder::stop() {
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

void H265Decoder::inputThread() {

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
        uint64_t timenow = timeUtils::getSystemUs();
        if (buffer->dts > timenow) {
            // 缓冲中
            ALOGD("[%s%d] caching! %ld, queueSize %lu", __FUNCTION__, __LINE__,
                  buffer->dts - timenow, mDataQueue->size());
            usleep(buffer->dts - timenow);
        }

        if (!bCheckSps) {
            if (checkIs_spsppsvps((unsigned char *) buffer->data())) {
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
                uint64_t presentationTimeUs = timeUtils::getSystemUs();
                ALOGD("[%s%d] AMediaCodec_queueInputBuffer one type:%d", __FUNCTION__, __LINE__,
                      getNaluType((unsigned char *) buffer->data()));
                AMediaCodec_queueInputBuffer(pMediaCodec, bufidx, 0,
                                             MIN(bufsize, buffer->size() + 4),
                                             presentationTimeUs, 0);
            }
        } while (bufidx < 0);
    }

}

void H265Decoder::outputThread() {
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

        } else if (bufidx == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {

        } else if (bufidx == AMEDIACODEC_CONFIGURE_FLAG_ENCODE) {

        } else {
            ALOGD("[%s%d] err: %ld", __FUNCTION__, __LINE__, bufidx);
            ALOGW("[%s%d] stop  the mediacodec ", __FUNCTION__, __LINE__);
            stop();
        }
    }
}

char H265Decoder::getNaluType(const unsigned char *buff) {
// h265
// type 第 1 到 6 位
    char type = ((buff[0] & (0x7E)) >> 1);
    return type;
}

bool H265Decoder::checkIs_spsppsvps(const unsigned char *buff) {
    char type = getNaluType(buff);
// 32 vps , 33 sps, 34 pps, 39 sei
    return (type == 32 || type == 33 || type == 34);
}