//
// Created by canok on 2022/7/21.
//

#ifndef LIVE555_OBOERENDER_H
#define LIVE555_OBOERENDER_H

#include "oboe/AudioStreamCallback.h"
#include "oboe/AudioStream.h"

class OboeRender : public oboe::AudioStreamCallback {
public:
    OboeRender();

    virtual ~OboeRender();

    oboe::DataCallbackResult
    onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override;

    bool onError(oboe::AudioStream * /* audioStream */, oboe::Result /* error */) override;


    void start(int samplerate, int ch, std::shared_ptr<MediaQueue> queue);

private:
    std::shared_ptr<oboe::AudioStream> mStream;
    std::shared_ptr<MediaQueue> mQueue;
    std::shared_ptr<MediaBuffer> mCurBuffer;
    int mCurPostion = 0; //这个值应该设计到MediaBuffer里面去
    int mCurRemainSize = 0;
private:
    size_t getBufferInternal(uint8_t **buf, const size_t wantSize);
};


#endif //LIVE555_OBOERENDER_H
