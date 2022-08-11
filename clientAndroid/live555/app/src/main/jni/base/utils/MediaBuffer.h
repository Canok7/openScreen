//
// Created by canok on 2022/7/21.
//

#ifndef LIVE555_MEDIABUFFER_H
#define LIVE555_MEDIABUFFER_H

#include<map>
#include<string>

class MediaBuffer {
public:
    MediaBuffer(size_t size);

    ~MediaBuffer();

    size_t size() const;

    void setParams(const std::string &key, int value);

    void setParams(const std::string &key, double value);

    void setParams(const std::string &key, std::string value);

    std::map<std::string, int> mIntPrams;
    std::map<std::string, double> mDoubleParams;
    std::map<std::string, std::string> mStringParams;

    void *data();

    std::string mCodecName;
    uint64_t durationUs = 0;
    uint64_t dts = 0;
    struct videoInfo {
        uint32_t w;
        uint32_t h;
        float fps;
    };
    struct audioInfo {
        uint32_t sampleRate;
        uint32_t channels;
    };
    union {
        videoInfo vInfo;
        audioInfo aInfo;
    } info{};
private:
    size_t mSize;
    void *mData;
};


#endif //LIVE555_MEDIABUFFER_H
