//
// Created by canok on 2022/8/8.
//

#ifndef LIVE555_VIDEODECODERINTERFACE_H
#define LIVE555_VIDEODECODERINTERFACE_H

#include <memory>

struct ANativeWindow;

class MediaQueue;

class VideoDecoderInterface {
public:
    virtual ~VideoDecoderInterface() = default;

    virtual int
    start(ANativeWindow *wind, const std::shared_ptr<MediaQueue> &dataQueues, char *workdir) = 0;

    virtual void stop() = 0;
};


#endif //LIVE555_VIDEODECODERINTERFACE_H
