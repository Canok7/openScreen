#ifndef  __LIVE555_HEAD_H__
#define __LIVE555_HEAD_H__
#include "queue.h"

class ourRTSPClient;
class UsageEnvironment;
class IRtspClientDestoryNotifyer{
public:
    IRtspClientDestoryNotifyer() =default;
    virtual  ~IRtspClientDestoryNotifyer(){}
    virtual void onRtspClinetDestoryed(void *) = 0;
};
class SrcLive555 :public IRtspClientDestoryNotifyer{
public:
    SrcLive555(char* workdir);
    ~SrcLive555();
    void start(char *url, bool bTcp, unsigned  udpReorderTimeUs);
    void stop();
    CQueue *getDataQueue();
    const char *getUrl() const;
    ourRTSPClient *getOurRtspClient();
    char *getEventLoopWatchVariable();
public:
    void openURL(UsageEnvironment &env, char const *progName, char const *rtspURL);
private:
    CQueue *mQueue= nullptr;
    char *mWorkdir= nullptr;
    char *mUrl= nullptr;
    pthread_t pliveThread = -1;
    ourRTSPClient *mRtspClient = nullptr;
    char eventLoopWatchVariable = 0;
    bool mBTcp=false;
    unsigned mUdpReorderingHoldTime;
private:
    void onRtspClinetDestoryed(void *);
};
#endif