//
// Created by canok on 2022/7/21.
//
#define LOG_TAG "MediaBuffer"

#include "logs.h"
#include "MediaBuffer.h"

#include <utility>


size_t MediaBuffer::size() const{
    return mSize;
}

void *MediaBuffer::data() {
    return mData;
}

void MediaBuffer::setParams(const std::string &key, int value) {
    mIntPrams[key] = value;
}

void MediaBuffer::setParams(const std::string &key, double value) {
    mDoubleParams[key] = value;
}

void MediaBuffer::setParams(const std::string &key, std::string value) {
    mStringParams[key] = std::move(value);
}

MediaBuffer::MediaBuffer(size_t size) : mSize(size) {
    mData = (void *) malloc(mSize);
}

MediaBuffer::~MediaBuffer() {
    free(mData);
    mData = nullptr;
}