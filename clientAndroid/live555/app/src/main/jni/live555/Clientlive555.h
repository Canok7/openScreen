#ifndef  __LIVE555_HEAD_H__
#define __LIVE555_HEAD_H__

#include "base/utils/queue.h"
#include <map>
#include <UsageEnvironment.hh>
#include "base/utils/MediaQueue.h"

class ourRTSPClient;

class UsageEnvironment;


class IRtspClientNotifyer {
public:
    IRtspClientNotifyer() = default;

    virtual  ~IRtspClientNotifyer() {}

    virtual void onRtspClinetDestoryed(void *) = 0;

    virtual void onNewStreamReady(std::shared_ptr<MediaQueue>) = 0;
};


class Clientlive555 : public IRtspClientNotifyer {
public:
    enum SRC_CMD {
        PAUSE_SUBSESSION,
        PLAY_SUBSESSION,
        TEARDOWN_SUBSESSION,
        CMD_UNKNOWN,
    };

    explicit Clientlive555(char *workdir);

    ~Clientlive555();

    void start(char *url, IRtspClientNotifyer *notifyer, bool bTcp, unsigned udpReorderTimeUs,
               unsigned cachTimeUs);

    void stop();

    void control(SRC_CMD cmd, void *data);

    const char *getUrl() const;

    ourRTSPClient *getOurRtspClient();

    char *getEventLoopWatchVariable();

    void setCmdTrigId(EventTriggerId id);

public:
    void openURL(UsageEnvironment &env, char const *progName, char const *rtspURL);

    SRC_CMD getCmd() {
        return mCmd;
    }

    void *getCmdData() {
        return mCmdData;
    }

private:
    std::map<std::string, std::shared_ptr<MediaQueue>> mQueues;
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

    EventTriggerId mCmdEventId = -1;
    SRC_CMD mCmd = CMD_UNKNOWN;
    void *mCmdData = nullptr;
private:
    void onRtspClinetDestoryed(void *) override;

    void onNewStreamReady(std::shared_ptr<MediaQueue>) override;
};

#endif