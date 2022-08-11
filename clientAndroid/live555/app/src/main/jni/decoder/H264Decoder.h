//
// Created by canok on 2021/8/1.
//

#ifndef LIVE555_H264DECODER_H
#define LIVE555_H264DECODER_H

#include "VideoDecoderInterface.h"
#include "media/NdkMediaCodec.h"
#include "media/NdkMediaFormat.h"
#include "base/utils/queue.h"
#include "base/utils/MediaQueue.h"
class H264Decoder : public VideoDecoderInterface{
public :
    H264Decoder();

    ~H264Decoder();

    int start(ANativeWindow *wind, const std::shared_ptr<MediaQueue> &dataQueues, char *workdir) override;

    void stop() override;

    void inputThread();

    void outputThread();

    static bool checkIs_spsppsNal(const unsigned char *buf);

private:
    AMediaCodec *pMediaCodec;
    pthread_t mThreadIn;
    pthread_t mThreadOut;

    std::shared_ptr<MediaQueue> mDataQueue=nullptr;
    bool bRun;
    bool bCheckSps;
};


#endif //LIVE555_H264DECODER_H
