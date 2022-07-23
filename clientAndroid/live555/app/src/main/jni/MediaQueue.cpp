//
// Created by canok on 2022/7/21.
//
#define LOG_TAG "MediaQueue"

#include "logs.h"
#include "MediaQueue.h"

MediaQueue::MediaQueue(const std::string &codecName, size_t maxSize) : mCodecName(codecName),
                                                                mMaxSize(maxSize) {
    ALOGD("[%s%d] %s", __FUNCTION__, __LINE__, codecName.c_str());
}

MediaQueue::~MediaQueue() {
    ALOGD("[%s%d] %s", __FUNCTION__, __LINE__, codecName().c_str());
}

std::string MediaQueue::codecName() {
    return mCodecName;
}

void MediaQueue::push(const std::shared_ptr<MediaBuffer> &buffer) {
    std::unique_lock<std::mutex> lk(opMutex);
    if (mQuue.size() >= mMaxSize) {
        ALOGI("[%s%d] %s cover!!!!", __FUNCTION__, __LINE__, codecName().c_str());
        mQuue.pop();
    }
    mQuue.push(buffer);

//    ALOGD("[%s%d]%s %lu",__FUNCTION__ ,__LINE__,codecName().c_str(),mQuue.size());
}

std::shared_ptr<MediaBuffer> MediaQueue::pop() {
//    ALOGD("[%s%d] %s",__FUNCTION__ ,__LINE__,codecName().c_str());
    std::unique_lock<std::mutex> lk(opMutex);
    if (mQuue.empty()) {
        return nullptr;
    }
    std::shared_ptr<MediaBuffer> buffer = mQuue.front();
    mQuue.pop();
    return buffer;
}

size_t MediaQueue::size() {
    std::unique_lock<std::mutex> lk(opMutex);
    return mQuue.size();
}

bool MediaQueue::empty() {
    std::unique_lock<std::mutex> lk(opMutex);
    return mQuue.empty();
}

std::shared_ptr<MediaBuffer> MediaQueue::front() {
    std::unique_lock<std::mutex> lk(opMutex);
    return mQuue.front();
}

std::shared_ptr<MediaBuffer> MediaQueue::back() {
    std::unique_lock<std::mutex> lk(opMutex);
    return mQuue.back();
}