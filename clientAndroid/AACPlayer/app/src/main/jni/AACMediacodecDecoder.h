//
// Created by xiancan.wang on 8/13/21.
//

#ifndef AACPLAYER_AACMEDIACODECDECODER_H
#define AACPLAYER_AACMEDIACODECDECODER_H

#include "IAudioRender.h"
#include "media/NdkMediaCodec.h"
#include "media/NdkMediaFormat.h"
class AACMediacodecDecoder {
public :
    AACMediacodecDecoder();
    ~AACMediacodecDecoder();
    int init(int chanle,int sampleFrequency,int sampleFreIndex,int profile);
    int start(IAudioRender *render);
    int stop();
    int dodecode(unsigned char*data,int len);
    void run();
private:
    bool bRun;
    AMediaCodec  *pMediaCodec;
    IAudioRender *pRender;
    pthread_t pidRender;
};


#endif //AACPLAYER_AACMEDIACODECDECODER_H
