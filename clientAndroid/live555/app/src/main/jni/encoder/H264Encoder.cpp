//
// Created by Administrator on 2022/8/11.
//
#define LOG_TAG "H264Encoder"

#include <utils/logs.h>
#include "H264Encoder.h"
#include <media/NdkMediaFormat.h>

#include "utils/MediaBuffer.h"
#include "utils/MediaQueue.h"

#include "Stream/Stream2File.h"

H264Encoder::H264Encoder(std::shared_ptr<SteamSinkerInterface> sink) : mSink(std::move(sink)) {

}

H264Encoder::~H264Encoder() {
    if (mThread != nullptr) {
        mThread->join();
    }
//    ALOGD("[%s%d]",__FUNCTION__ ,__LINE__);
}

status_t H264Encoder::initInternal() {
    mAMediaCodec = AMediaCodec_createEncoderByType("video/avc");//h264 //
    AMediaFormat *format = AMediaFormat_new();
    AMediaFormat_setString(format, "mime", "video/avc");
//        AMediaFormat_setInt32(format, "color-format", OMX_COLOR_FormatAndroidOpaque);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, mConfig.w);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, mConfig.h);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, mConfig.bitRate);
    AMediaFormat_setInt32(format, "i-frame-interval", 2);
    AMediaFormat_setInt32(format, "prepend-sps-pps-to-idr-frames", 1);
    AMediaFormat_setInt32(format, "frame-rate", mConfig.fps);

    media_status_t status = AMediaCodec_configure(mAMediaCodec, format, nullptr, nullptr,
                                                  AMEDIACODEC_CONFIGURE_FLAG_ENCODE);//解码，flags 给0，编码给AMEDIACODEC_CONFIGURE_FLAG_ENCODE
    if (AMEDIA_ERROR_BASE == status) {
        AMediaFormat_setInt32(format, "prepend-sps-pps-to-idr-frames", 0);
        status = AMediaCodec_configure(mAMediaCodec, format, nullptr, nullptr,
                                       AMEDIACODEC_CONFIGURE_FLAG_ENCODE);//解码，flags 给0，编码给AMEDIACODEC_CONFIGURE_FLAG_ENCODE
    }

    if (status != 0) {
        ALOGE("skype erro config %d\n", status);
        AMediaFormat_delete(format);
        return INVALID_OPERATION;
    }

    status = AMediaCodec_createInputSurface(mAMediaCodec, &mANateWindow);
    if (status != OK) {
        ALOGD("[%s%d]createInput status:%d", __FUNCTION__, __LINE__, status);
        AMediaFormat_delete(format);
        return INVALID_OPERATION;
    }

    AMediaFormat_delete(format);
    return OK;
}

status_t H264Encoder::frameProce() {
    AMediaCodecBufferInfo info;
    auto outindex = AMediaCodec_dequeueOutputBuffer(mAMediaCodec, &info, 2000);
    if (outindex >= 0) {
        size_t outsize;
        uint8_t *buf = AMediaCodec_getOutputBuffer(mAMediaCodec, outindex, &outsize);
        if (nullptr != buf) {
            std::shared_ptr<MediaBuffer> abuff = std::make_shared<MediaBuffer>(info.size);
            memcpy(abuff->data(), buf, info.size);
//            if(mQueue == nullptr){
//                mQueue = std::make_shared<MediaQueue>("h264");
//            }
//            if(mQueue){
//                mQueue->push(abuff);
//            }
            if (mSink == nullptr) {
                ALOGI("[%s%d]not set sink , write to a file!", __FUNCTION__, __LINE__);
                mSink = std::make_shared<Stream2File>();
                StreamSinkerInfo streamInfo = {.name="h264"};
                mSink->init(mWorkdir, streamInfo);
            }
            mSink->pushOneFrame(abuff);
        }
        AMediaCodec_releaseOutputBuffer(mAMediaCodec, outindex, false);
        //ALOGD("[%s%d]xiancan_get AFrame %d \n",__FUNCTION__,__LINE__,info.size);
    } else if (outindex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
        AMediaFormat *format2 = AMediaCodec_getOutputFormat(mAMediaCodec);
        int32_t frameRate, w, h;
        AMediaFormat_getInt32(format2, AMEDIAFORMAT_KEY_FRAME_RATE, &frameRate);
        AMediaFormat_getInt32(format2, AMEDIAFORMAT_KEY_WIDTH, &w);
        AMediaFormat_getInt32(format2, AMEDIAFORMAT_KEY_HEIGHT, &h);
        ALOGD("AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED ---pts %lld %dX%d@%d %d pthread:%ld\n",
              (long long) info.presentationTimeUs, w, h, frameRate, info.size, pthread_self());
    }
    return OK;
}

status_t H264Encoder::start(const VideoEncoderConfig &config, std::string &workdir) {

    mConfig = config;
    mWorkdir = workdir;

    status_t ret = initInternal();
    if (ret == OK) {
        ret = AMediaCodec_start(mAMediaCodec);
    }

    auto thread_run = [=, function_name = "thread_run"]() {
        status_t ret = OK;
        while (bRun && ret == OK) {
            ret = frameProce();
        }

        AMediaCodec_stop(mAMediaCodec);
        AMediaCodec_delete(mAMediaCodec);
        ALOGD("[%s%d] encoder thread exit...........", function_name, __LINE__);
        return OK;
    };

    mThread = std::make_unique<std::thread>(thread_run);
    return true;
}

void H264Encoder::stop() {
    bRun = false;
}

ANativeWindow *H264Encoder::getInputSurface() {
    return mANateWindow;
}