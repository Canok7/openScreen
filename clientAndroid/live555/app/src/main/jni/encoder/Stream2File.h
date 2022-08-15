//
// Created by canok on 2022/8/12.
//

#ifndef LIVE555_STREAM2FILE_H
#define LIVE555_STREAM2FILE_H

#include <thread>
#include "SteamSinkerInterface.h"

class Stream2File : public SteamSinkerInterface{
public:
    Stream2File() = default;
    virtual  ~Stream2File();
    void start(std::shared_ptr<MediaQueue> &queue, std::string &workdir) override;

private:
    FILE *mfp = nullptr;
    std::unique_ptr<std::thread> mThread = nullptr;
    std::shared_ptr<MediaQueue> mQueue;
    bool  bRun = true;
};


#endif //LIVE555_STREAM2FILE_H
