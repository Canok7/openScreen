//
// Created by Administrator on 2021/8/1.
//

#ifndef LIVE555_DECODER_H
#define LIVE555_DECODER_H

#include "media/NdkMediaCodec.h"
#include "media/NdkMediaFormat.h"
#include "logs.h"
#include "queue.h"
class Decoder {
public :
    Decoder();
    ~Decoder();
    int start(ANativeWindow *wind,CQueue *dataQueue, char* workdir);
    void stop();
    void inputThread();
    void outputThread();
    static bool checkIs_spsppsNal(unsigned char*buf);
private:
    AMediaCodec  *pMediaCodec;
    pthread_t mThreadIn;
    pthread_t mThreadOut;
    CQueue *mDataQueue;
    bool bRun;
    bool bCheckSps;
};


#endif //LIVE555_DECODER_H
