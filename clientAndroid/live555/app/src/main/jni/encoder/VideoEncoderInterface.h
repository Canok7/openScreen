//
// Created by canok on 2022/8/12.
//

#ifndef LIVE555_VIDEOENCODERINTERFACE_H
#define LIVE555_VIDEOENCODERINTERFACE_H
struct VideoEncoderConfig{
    int w;
    int h;
    int bitRate;
    float fps;
};
class VideoEncoderInterface{
public:
    virtual status_t start(const VideoEncoderConfig &config,  std::string &workdir ) = 0;

    virtual void stop() = 0;

    virtual  ANativeWindow * getInputSurface() =0;
};

#endif //LIVE555_VIDEOENCODERINTERFACE_H
