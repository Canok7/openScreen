//
// Created by canok on 2022/7/20.
//

#define LOG_TAG "AACDecoder"

#include "logs.h"
#include <utils/aacUtils.h>
#include <utils/timeUtils.h>
#include <unistd.h>
#include "AACDecoder.h"


#define MIN(a, b) (a)<(b)?(a):(b)

AACDecoder::~AACDecoder() {
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
    stop();
    if (pMediaCodec) {
        AMediaCodec_delete(pMediaCodec);
        pMediaCodec = nullptr;
    }
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
}

static void *input(void *privdata) {
    auto *decoder = (AACDecoder *) privdata;
    decoder->inputThread();
    return nullptr;
}

static void *output(void *privdata) {
    auto *decoder = (AACDecoder *) privdata;
    decoder->outputThread();
    return nullptr;
}

int AACDecoder::start(const std::shared_ptr<MediaQueue> &dataQueues, char *workdir) {
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
    if (dataQueues == nullptr) {
        ALOGE("[%s%d] start err! dataQueue == nullptr, parame err!", __FUNCTION__, __LINE__);
        return -1;
    }
#if 0
    char file_out[128]={0};
    sprintf(file_out,"%s/out.pcm",workdir);
    mfp = fopen(file_out,"w+");
    if(mfp==nullptr){
        ALOGE("[%s%d] fopen err:%s",__FUNCTION__ ,__LINE__,file_out);
    }
#else
    (void) mfp;
#endif

    mSrcQueue = dataQueues;
    std::shared_ptr<MediaBuffer> buffer = nullptr;
    while (buffer == nullptr) {
        buffer = dataQueues->pop();
    }

    mSampleRate = buffer->info.aInfo.sampleRate;
    mCh = buffer->info.aInfo.channels;
    bRun = true;
    if (pMediaCodec == nullptr) {
        pMediaCodec = AMediaCodec_createDecoderByType("audio/mp4a-latm");//aac
        AMediaFormat *format = AMediaFormat_new();

        AMediaFormat_setString(format, "mime", "audio/mp4a-latm");
        // 如果出错，要调整这个参数    0 1 2
        int profile = 1;
        int sampleFreIndex = 0;
        static const int32_t kSamplingFreq[] = {
                96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
                16000, 12000, 11025, 8000
        };
        for (int i = 0; i < sizeof(kSamplingFreq) / sizeof(kSamplingFreq[0]); i++) {
            if (mSampleRate == kSamplingFreq[i]) {
                sampleFreIndex = i;
            }
        }
        ALOGD("aaccode[%s%d] sampleFreIndex %d,sampleRate %d, ch %d, profile %d", __FUNCTION__,
              __LINE__, sampleFreIndex, mSampleRate, mCh, profile);
        aacUtils::MakeAACCodecSpecificData(format, profile, sampleFreIndex, mCh);
        //用来标记AAC是否有adts头，1->有
        AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_IS_ADTS, 0);
        media_status_t status = AMediaCodec_configure(pMediaCodec, format, nullptr, nullptr, 0);
        AMediaFormat_delete(format);
        if (status != 0) {
            ALOGE("aaccode erro config %d", status);
            return -1;
        }
    }

    AMediaCodec_start(pMediaCodec);
    //也可以通过 AMediaCodec_setAsyncNotifyCallback 设置成异步方式.
    if (pthread_create(&mThreadIn, nullptr, input, (void *) this) != 0) {
        ALOGE("aaccode [%s%d] pthread create err!!", __FUNCTION__, __LINE__);
        return -1;
    }
    if (pthread_create(&mThreadOut, nullptr, output, (void *) this) != 0) {
        ALOGE("aaccode [%s%d] pthread create err!!", __FUNCTION__, __LINE__);
        return -1;
    }

    return 0;
}

void AACDecoder::stop() {
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
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
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
}

void AACDecoder::inputThread() {
    while (bRun) {
        if (mSrcQueue == nullptr) {
            assert(mSrcQueue != nullptr);
            ALOGE("[%s%d] erro!!", __FUNCTION__, __LINE__);
        }
        if (mSrcQueue->empty()) {
            //避免空转，还是休眠一下
//                ALOGD("[%s%d] ",__FUNCTION__ ,__LINE__);
            usleep(1000);
            continue;
        }

        std::shared_ptr<MediaBuffer> buffer = mSrcQueue->pop();
        if (nullptr == buffer) {
            continue;
        }

        uint64_t timenow = timeUtils::getSystemUs();
        if (buffer->dts > timenow) {
            // 缓冲中
            ALOGD("[%s%d] caching! %ld, queueSize %lu", __FUNCTION__, __LINE__,
                  buffer->dts - timenow, mSrcQueue->size());
            usleep(buffer->dts - timenow);
        }

        ssize_t bufidx = 0;
        do {
            bufidx = AMediaCodec_dequeueInputBuffer(pMediaCodec, 1000 * 20);
            if (bufidx >= 0) {
                size_t bufsize;
                uint8_t *buf = AMediaCodec_getInputBuffer(pMediaCodec, bufidx, &bufsize);
                if (bufsize < buffer->size()) {
                    ALOGE("[%s%d]something maybe erro! will drop some video data!!! please check the mediacodec or src data:bufsize:%lu,datalen:%lu",
                          __FUNCTION__, __LINE__, bufsize, buffer->size());
                }
                memcpy(buf, buffer->data(), MIN(bufsize, buffer->size()));
                uint64_t presentationTimeUs = timeUtils::getSystemUs();
//                    ALOGD("aac [%s%d] AMediaCodec_queueInputBuffer one %lu ", __FUNCTION__, __LINE__, buffer->size());
                AMediaCodec_queueInputBuffer(pMediaCodec, bufidx, 0, MIN(bufsize, buffer->size()),
                                             presentationTimeUs, 0);
            }
        } while (bufidx < 0);
    }
}

void AACDecoder::outputThread() {
    while (bRun) {
        size_t bufsize;
        AMediaCodecBufferInfo info;
        ssize_t bufidx = 0;
        bufidx = AMediaCodec_dequeueOutputBuffer(pMediaCodec, &info, 1000 * 20);
        if (bufidx >= 0) {
            uint8_t *buf = AMediaCodec_getOutputBuffer(pMediaCodec, bufidx, &bufsize);
            std::shared_ptr<MediaBuffer> buffer = std::make_shared<MediaBuffer>(info.size);
            memcpy(buffer->data(), buf, info.size);

            if (nullptr == mRenderQueue) {
                mRenderQueue = std::make_shared<MediaQueue>("pcm");
            }
            if (nullptr == mRender) {
                mRender = std::make_shared<OboeRender>();
                mRender->start(mSampleRate, mCh, mRenderQueue);
            }
            mRenderQueue->push(buffer);
//            ALOGD("[%s%d] info.size:%d, bufsize:%lu",__FUNCTION__ ,__LINE__,info.size,bufsize);
//            if(mfp) {
//                ALOGD("aaccode write[%s%d]",__FUNCTION__,__LINE__);
//                fwrite(buf, 1, info.size, mfp);
//            }
            AMediaCodec_releaseOutputBuffer(pMediaCodec, bufidx, false);
        } else if (bufidx == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
        } else if (bufidx == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
        } else {
        }
    }

    mRender.reset();
}