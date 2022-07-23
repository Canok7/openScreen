//
// Created by canok on 2022/7/20.
//

#ifndef LIVE555_AACDECODER_H
#define LIVE555_AACDECODER_H


#include <MediaQueue.h>
#include <media/NdkMediaCodec.h>
#include "Render/OboeRender.h"

class AACDecoder {
public :
    AACDecoder() = default;

    ~AACDecoder();

    int start(const std::shared_ptr<MediaQueue> &dataQueues, char *workdir);

    void stop();

    void inputThread();

    void outputThread();

private:
    AMediaCodec *pMediaCodec = nullptr;
    pthread_t mThreadIn = -1;
    pthread_t mThreadOut = -1;
    std::shared_ptr<MediaQueue> mSrcQueue = nullptr;
    std::shared_ptr<MediaQueue> mRenderQueue = nullptr;
    bool bRun = true;
    FILE *mfp = nullptr;
    int mSampleRate = 0;
    int mCh = 0;

    std::shared_ptr<OboeRender> mRender = nullptr;
};


#endif //LIVE555_AACDECODER_H
