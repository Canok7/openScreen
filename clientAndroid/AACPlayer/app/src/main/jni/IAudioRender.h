//
// Created by xiancan.wang on 8/13/21.
//

#ifndef AACPLAYER_IAUDIORENDER_H
#define AACPLAYER_IAUDIORENDER_H


class IAudioRender {
public:
    IAudioRender(){}
    ~IAudioRender(){}
    virtual int init(int sampleRateInHz, int channelConfig,int sampleDepth)=0;
    virtual void play(unsigned char *buf,int size)=0;
};


#endif //AACPLAYER_IAUDIORENDER_H
