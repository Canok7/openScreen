//
// Created by 8216337 on 2022/7/21.
//

#ifndef LIVE555_MEDIAQUEUE_H
#define LIVE555_MEDIAQUEUE_H

#include<string>
#include<queue>
#include<mutex>
#include "MediaBuffer.h"

class MediaQueue {
public:
    MediaQueue(const std::string &codecName, size_t maxSize = 400);

//    ~MediaQueue() = default;
    ~MediaQueue();

    std::string codecName();

    void push(const std::shared_ptr<MediaBuffer>&);

    std::shared_ptr<MediaBuffer> pop();

    size_t size();

    bool empty();

    std::shared_ptr<MediaBuffer> front();

    std::shared_ptr<MediaBuffer> back();

private:
    std::string mCodecName;
    std::queue<std::shared_ptr<MediaBuffer>> mQuue;
    std::mutex opMutex;
    size_t mMaxSize;
};


#endif //LIVE555_MEDIAQUEUE_H
