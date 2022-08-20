//
// Created by Administrator on 2022/8/16.
//

#ifndef LIVE555_H264STREAMFROMEFILE_H
#define LIVE555_H264STREAMFROMEFILE_H


#include "StreamSourceInterface.h"
#include <utils/MediaQueue.h>

class h264StreamFromeFile : public SteamSourceInterface {
public:
    h264StreamFromeFile(const std::string& filename , bool bLoop = true);

    virtual ~h264StreamFromeFile();

    void init(std::string &workdir) override;

    std::shared_ptr<MediaBuffer> getOneFrame() override;

private:
    FILE *mfp = nullptr;
    uint8_t *mBuffers[2];
    int icach = 0;
    int ioffset = 0;
    bool bLoop = true;
private:
    std::shared_ptr<MediaBuffer> h264_getOneNal();
};


#endif //LIVE555_H264STREAMFROMEFILE_H
