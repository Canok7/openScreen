#ifndef  __LIVE555_HEAD_H__
#define __LIVE555_HEAD_H__

#include "queue.h"
#include <map>
#include "MediaQueue.h"
class ourRTSPClient;

class UsageEnvironment;



class IRtspClientNotifyer {
public:
    IRtspClientNotifyer() = default;

    virtual  ~IRtspClientNotifyer() {}

    virtual void onRtspClinetDestoryed(void *) = 0;

    virtual void onNewStreamReady(std::shared_ptr<MediaQueue> ) = 0;
};



class SrcLive555 : public IRtspClientNotifyer {
public:
    SrcLive555(char *workdir);

    ~SrcLive555();

    void start(char *url, IRtspClientNotifyer *notifyer, bool bTcp, unsigned udpReorderTimeUs, unsigned cachTimeUs);

    void stop();

    const char *getUrl() const;

    ourRTSPClient *getOurRtspClient();

    char *getEventLoopWatchVariable();

public:
    void openURL(UsageEnvironment &env, char const *progName, char const *rtspURL);

private:
    std::map<std::string, std::shared_ptr<MediaQueue>>  mQueues;
    char *mWorkdir = nullptr;
    char *mUrl = nullptr;
    pthread_t pliveThread = -1;
    ourRTSPClient *mRtspClient = nullptr;
    char eventLoopWatchVariable = 0;
    bool mBTcp = false;
    unsigned mUdpReorderingHoldTime = 0;
    unsigned mCachingTimeUs = 0;

    //用于回调
    IRtspClientNotifyer *mNotifyer = nullptr;
private:
    void onRtspClinetDestoryed(void *) override;
    void onNewStreamReady(std::shared_ptr<MediaQueue> ) override;
};

#endif