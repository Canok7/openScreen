//
// Created by canok on 2022/8/16.
//

#ifndef LIVE555_STREAMSOURCEINTERFACE_H
#define LIVE555_STREAMSOURCEINTERFACE_H

#include <memory>

class MediaQueue;

class MediaBuffer;

class SteamSourceInterface {
public:
    virtual void init(std::string &workdir) = 0;

    virtual std::shared_ptr<MediaBuffer> getOneFrame() = 0;
};

#endif //LIVE555_STREAMSOURCEINTERFACE_H
