//
// Created by canok on 2022/8/11.
//

#ifndef LIVE555_H264ENCODER_H
#define LIVE555_H264ENCODER_H

#include <media/NdkMediaCodec.h>
#include <thread>
#include "Errors.h"
#include "VideoEncoderInterface.h"
#include "Stream/SteamSinkerInterface.h"

class MediaQueue;
class H264Encoder : public VideoEncoderInterface{
public:
    H264Encoder(std::shared_ptr<SteamSinkerInterface> sink = nullptr) ;
    virtual  ~H264Encoder();
    status_t start(const VideoEncoderConfig &config , std::string &workdir) override;

    void stop() override;

    ANativeWindow * getInputSurface() override;

private:
    status_t  initInternal();
    status_t frameProce();

    std::unique_ptr< std::thread > mThread;
    AMediaCodec* mAMediaCodec = nullptr;
    ANativeWindow *mANateWindow = nullptr;
    VideoEncoderConfig mConfig ;
    bool  bRun = true;
//    std::shared_ptr<MediaQueue> mQueue;
    std::shared_ptr<SteamSinkerInterface> mSink = nullptr;
    std::string mWorkdir;
};


#endif //LIVE555_H264ENCODER_H
