//
// Created by canok on 2022/8/20.
//

#ifndef LIVE555_PATCH_H
#define LIVE555_PATCH_H

#include "SteamSinkerInterface.h"
#include "StreamSourceInterface.h"


// patch  类似与Android AudioPatch  用来连接source和sinker的
class Patch : public SteamSinkerInterface, public SteamSourceInterface {
public:
    Patch();

    virtual ~Patch() = default;

    void init(std::string &workdir, const StreamSinkerInfo &info) override;

    void pushOneFrame(std::shared_ptr<MediaBuffer>) override;

    void init(std::string &workdir) override;

    std::shared_ptr<MediaBuffer> getOneFrame() override;

private:
    std::unique_ptr<MediaQueue> mQueue = nullptr;
};


#endif //LIVE555_PATCH_H
