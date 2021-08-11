//
// Created by xiancan.wang on 8/10/21.
//

#ifndef AACPLAYER_GETAACFRAME_H
#define AACPLAYER_GETAACFRAME_H
#include<stdio.h>
#include<stdlib.h>

class getAACFrame {
public:
    getAACFrame(const char*file,bool loop=true);
    ~getAACFrame();
    int getOneFrameWithoutADTS(unsigned char *dest,int buflen);
    int getOneFrameFull(unsigned char *dest,int buflen);
    int probeInfo(int *channel, int *samplingFrequency,int *sampleFreInd,int *iprofile);
private:
    FILE *fp_in;
    int sample;
    int channel;
    bool bLoop;
};


#endif //AACPLAYER_GETAACFRAME_H
