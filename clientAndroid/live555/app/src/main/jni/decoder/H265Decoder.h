//
// Created by Administrator on 2022/8/6.
//

#ifndef LIVE555_H265DECODER_H
#define LIVE555_H265DECODER_H

#include "media/NdkMediaCodec.h"
#include "media/NdkMediaFormat.h"
#include "utils/queue.h"
#include "utils/MediaQueue.h"
#include "VideoDecoderInterface.h"
class H265Decoder : public VideoDecoderInterface{
public :
    H265Decoder();

    ~H265Decoder();

    int start(ANativeWindow *wind, const std::shared_ptr<MediaQueue> &dataQueues, char *workdir) override;

    void stop() override;

    void inputThread();

    void outputThread();

    static bool checkIs_spsppsvps(const unsigned char *buf);

    static char getNaluType(const unsigned char *buf);

private:
    AMediaCodec *pMediaCodec = nullptr;
    pthread_t mThreadIn = -1;
    pthread_t mThreadOut = -1;

    std::shared_ptr<MediaQueue> mDataQueue=nullptr;
    bool bRun = true;
    bool bCheckSps = false;
};


#endif //LIVE555_H265DECODER_H
