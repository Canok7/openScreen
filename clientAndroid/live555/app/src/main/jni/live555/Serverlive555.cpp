//
// Created by 8216337 on 2022/8/12.
//

#define LOG_TAG "ServerLive555"

#include <utils/logs.h>
#include <UsageEnvironment.hh>
#include <RTSPServer.hh>
#include <BasicUsageEnvironment.hh>
#include <H264VideoStreamDiscreteFramer.hh>
#include <H264VideoRTPSink.hh>
#include "OnDemandServerMediaSubsession.hh"

#include "Serverlive555.h"
#include <memory>
#include <thread>
#include <Stream/StreamSourceInterface.h>


#define MIN(a, b) (a)<(b)?(a):(b)

class H264FramedLiveSource : public FramedSource {

public:
    static H264FramedLiveSource *
    createNew(UsageEnvironment &env, std::shared_ptr<SteamSourceInterface> source) {
        auto *newSource = new H264FramedLiveSource(env, std::move(source));
        return newSource;
    }

    unsigned maxFrameSize() const override {
//shoud change #define BANK_SIZE   1024*300 if use H264VideoStreamFramer as frameSource
        return 1024 * 300;
    }

protected:

    H264FramedLiveSource(UsageEnvironment &env, std::shared_ptr<SteamSourceInterface> source)
            : FramedSource(
            env), mSource(std::move(source)) {

    }

    virtual ~H264FramedLiveSource() {

    }

private:
//    std::shared_ptr<MediaQueue> mQueue = nullptr;
    std::shared_ptr<SteamSourceInterface> mSource;
private:
    void doGetNextFrame() override {
        ALOGD("[%s%d]", __FUNCTION__, __LINE__);
//         //control speed    应该通过  fDurationInMicroseconds = fuSecsPerFrame; 来控制
        std::shared_ptr<MediaBuffer> buf = mSource->getOneFrame();
        if (buf == nullptr) {
            nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                                                     (TaskFunc *) getFrameAgain,
                                                                     this);
            return;
        }

        fFrameSize = MIN(buf->size(), fMaxSize);
        memcpy(fTo, buf->data(), fFrameSize);

        if (fFrameSize == 0) {
            handleClosure();
            return;
        }

        //set timestamp
        gettimeofday(&fPresentationTime, nullptr);

        // Inform the reader that he has data:
        // To avoid possible infinite recursion, we need to return to the event loop to do this:
        nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                                                 (TaskFunc *) FramedSource::afterGetting,
                                                                 this);
        return;
    }


    static void getFrameAgain(void *clientData) {
        auto source = (H264FramedLiveSource *) clientData;
        source->doGetNextFrame();
    }
};

class H264LiveVideoServerMediaSubssion : public OnDemandServerMediaSubsession {

public:
    static H264LiveVideoServerMediaSubssion *
    createNew(UsageEnvironment &env, std::shared_ptr<SteamSourceInterface> source,
              IRtspServerNotifyer *notify = nullptr) {
        return new H264LiveVideoServerMediaSubssion(env, source, notify);
    }

protected:
    H264LiveVideoServerMediaSubssion(UsageEnvironment &env,
                                     std::shared_ptr<SteamSourceInterface> source,
                                     IRtspServerNotifyer *notify)
            : OnDemandServerMediaSubsession(env,
                                            true), mSource(source), mNotify(notify) {
    }

    virtual ~H264LiveVideoServerMediaSubssion() = default;

protected: // redefined virtual functions
    FramedSource *createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate) override {
        ALOGD("[%s%d] %s ", __FUNCTION__, __LINE__, fSDPLines);
        ALOGD("[%s%d] clientSessionId %d ", __FUNCTION__, __LINE__, clientSessionId);
        if (nullptr != mNotify) {
            ClientInfo info;
            // TODO: 填充这个clientInfo
            mNotify->onNewClientConnected(info);
        }

        H264FramedLiveSource *liveSource = H264FramedLiveSource::createNew(envir(), mSource);
        if (liveSource == nullptr) {
            return nullptr;
        }

        return H264VideoStreamDiscreteFramer::createNew(envir(), liveSource);
    }

    RTPSink *createNewRTPSink(Groupsock *rtpGroupsock,
                              unsigned char rtpPayloadTypeIfDynamic,
                              FramedSource *inputSource) override {
        return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
    }

private:
    std::shared_ptr<SteamSourceInterface> mSource;
    IRtspServerNotifyer *mNotify = nullptr;
};


class OurRTSPServer : public RTSPServer {
public:
//    TODO:(canok) to get client info in here
    OurRTSPServer(UsageEnvironment &env, Port ourPort = 554) : RTSPServer(
            *RTSPServer::createNew(env, ourPort)) {

    }

    ~OurRTSPServer() override{
        ALOGD("[%s%d] ", __FUNCTION__, __LINE__);
    }
};

Serverlive555::Serverlive555(std::string workdir, std::shared_ptr<SteamSourceInterface> srouce)
        : mSource(
        srouce) {

}

Serverlive555::~Serverlive555() {
    stop();
}


static void announceStream(RTSPServer *rtspServer, ServerMediaSession *sms,
                           char const *streamName) {
    char *url = rtspServer->rtspURL(sms);
    UsageEnvironment &env = rtspServer->envir();
    env << "\n\"" << streamName << "\" stream, from the file \"" << "\"\n";
    env << "Play this stream using the URL \"" << url << "\"\n";

    ALOGI("[%s%d]Play this stream using the URL:%s", __FUNCTION__, __LINE__, url);
    delete[] url;
}

void Serverlive555::start(IRtspServerNotifyer *notifyer, int port) {
    mNotifyer = notifyer;
    auto thread_run = [=, function_name = "liveServerThread"]() {

        ALOGD("[%s%d] server thread start---", function_name, __LINE__);
        TaskScheduler *scheduler = BasicTaskScheduler::createNew();
        UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);

        // Create the RTSP server:
//        RTSPServer *rtspServer = RTSPServer::createNew(*env, port, nullptr);
        auto *rtspServer = new OurRTSPServer(*env, port);
        CHECK(rtspServer != nullptr);

        OutPacketBuffer::maxSize = 2000000;
        char const *streamName = "h264Live";
        char const *descriptionString
                = "Session streamed by \"testOnDemandRTSPServer\"";
        ServerMediaSession *sms
                = ServerMediaSession::createNew(*env, streamName, streamName,
                                                descriptionString);
        sms->addSubsession(H264LiveVideoServerMediaSubssion
                           ::createNew(*env, mSource, this));
        rtspServer->addServerMediaSession(sms);

        announceStream(rtspServer, sms, streamName);

        // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
        // Try first with the default HTTP port (80), and then with the alternative HTTP
        // port numbers (8000 and 8080).
        if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) ||
            rtspServer->setUpTunnelingOverHTTP(8080)) {
            *env << "\n(We use port " << rtspServer->httpServerPortNum()
                 << " for optional RTSP-over-HTTP tunneling.)\n";
        } else {
            *env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
        }

        env->taskScheduler().doEventLoop(&bStop);
        delete rtspServer;
        env->reclaim();
        delete scheduler;
        ALOGD("[%s%d] live555 work thread over!--", function_name, __LINE__);
    };

    if (mThread == nullptr) {
        bStop = 0;
        mThread = std::make_unique<std::thread>(thread_run);
    }
}

void Serverlive555::stop() {
    if (mThread) {
        bStop = 1;
        mThread->join();
        mThread.reset();
    }
}

void Serverlive555::onRtspClinetDestoryed(const ClientInfo &info) {
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
}

void Serverlive555::onNewClientConnected(const ClientInfo &info) {
    ALOGD("[%s%d]", __FUNCTION__, __LINE__);
    if (mNotifyer) {
        mNotifyer->onNewClientConnected(info);
    }
}