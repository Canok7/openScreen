//
// Created by canok on 2022/7/21.
//
#define LOG_TAG "oboeRender"

#include "logs.h"
#include <AudioStreamBuilder.h>

#include <MediaQueue.h>
#include "OboeRender.h"
#include "oboe/Utilities.h"
#include "utils/timeUtils.h"

OboeRender::OboeRender() = default;

OboeRender::~OboeRender() {
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
    if (mStream) {
        mStream->stop();
        mStream->close();
    }
}

oboe::DataCallbackResult
OboeRender::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {
    size_t toCopy = numFrames * mStream->getBytesPerFrame();
//    ALOGD("[%s%d] tocopy %lu, frames:%d",__FUNCTION__ ,__LINE__,toCopy,numFrames);
    size_t count = 0;
    uint8_t *src = nullptr;
    auto *dest = (uint8_t *) audioData;
    while (toCopy > 0) {
        count = getBufferInternal(&src, toCopy);
        if (count == 0) {//  // 不能阻塞 如果获取不到数据，给0，静音数据。
            ALOGW("[%s%d] underload!!!!!!! 音频可能卡顿", __FUNCTION__, __LINE__);
            count = toCopy;
            memset(dest, 0, count);
        } else {// 正常读取
            memcpy(dest, src, count);
        }
        toCopy -= count;
        dest += count;
    }
    return oboe::DataCallbackResult::Continue;
}

bool OboeRender::onError(oboe::AudioStream *audioStream, oboe::Result error) {
    ALOGD("[%s%d] %s ", __FUNCTION__, __LINE__, convertToText(error));
    return false;
}

void OboeRender::start(int samplerate, int ch, std::shared_ptr<MediaQueue> queue) {
    ALOGI("[%s%d] start render :samplerate:%d, ch %d,", __FUNCTION__, __LINE__, samplerate, ch);
    mQueue = queue;
    oboe::AudioStreamBuilder builder = oboe::AudioStreamBuilder();
    builder.setDirection(oboe::Direction::Output)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setFormat(oboe::AudioFormat::I16)
            ->setChannelCount(ch)
            ->setSampleRate(samplerate)
            ->setCallback(this)
            ->openStream(mStream);
    mStream->start();
}


size_t OboeRender::getBufferInternal(uint8_t **buf, const size_t wantSize) {
    if (nullptr == mCurBuffer) {
        mCurBuffer = mQueue->pop();
        if (mCurBuffer) {
            mCurPostion = 0;
            mCurRemainSize = mCurBuffer->size();
        } else {
            return 0;
        }
    }
    size_t takeSize = MIN(wantSize, mCurRemainSize);

    uint8_t *buffer = ((uint8_t *) mCurBuffer->data()) + mCurPostion;
    *buf = buffer;

    mCurRemainSize -= takeSize;
    mCurPostion += takeSize;
    if (mCurRemainSize <= 0) {
        mCurBuffer.reset();
    }

    return takeSize;
}