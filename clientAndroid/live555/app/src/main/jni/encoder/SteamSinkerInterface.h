//
// Created by canok on 2022/8/12.
//

#ifndef LIVE555_STEAMSINKERINTERFACE_H
#define LIVE555_STEAMSINKERINTERFACE_H

#include <memory>
class MediaQueue;
class SteamSinkerInterface {
public:
    virtual void start(std::shared_ptr<MediaQueue> &queue, std::string &workdir) = 0;
};


#endif //LIVE555_STEAMSINKERINTERFACE_H
