//
// Created by canok on 2022/8/12.
//

#ifndef LIVE555_STEAMSINKERINTERFACE_H
#define LIVE555_STEAMSINKERINTERFACE_H

#include <memory>
#include <string>

class MediaBuffer;

class MediaQueue;

struct StreamSinkerInfo {
    std::string name;
};

class SteamSinkerInterface {
public:
    virtual void init(std::string &workdir, const StreamSinkerInfo &info) = 0;

    virtual void pushOneFrame(std::shared_ptr<MediaBuffer>) = 0;
};


#endif //LIVE555_STEAMSINKERINTERFACE_H
