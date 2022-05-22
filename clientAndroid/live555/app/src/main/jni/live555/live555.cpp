/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2017, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/
#include <pthread.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include "live555.h"
#include "logs.h"
#include "groupsock/GroupsockHelper.hh"
#include "liveMedia/liveMedia.hh"
#include "BasicUsageEnvironment/BasicUsageEnvironment.hh"
#include "UsageEnvironment/UsageEnvironment.hh"
// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString);

void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString);

void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString);

// Other event handler functions:
void subsessionAfterPlaying(
        void *clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void
subsessionByeHandler(void *clientData); // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void *clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient *rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient *rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment &operator<<(UsageEnvironment &env, const RTSPClient &rtspClient) {
    return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment &operator<<(UsageEnvironment &env, const MediaSubsession &subsession) {
    return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment &env, char const *progName) {
    env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
    env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}
//-----------------

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:
class StreamClientState {
public:
    StreamClientState();

    virtual ~StreamClientState();

public:
    MediaSubsessionIterator *iter;
    MediaSession *session;
    MediaSubsession *subsession;
    TaskToken streamTimerTask;
    double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:


class ourRTSPClient : public RTSPClient {
public:
    static ourRTSPClient *createNew(UsageEnvironment &env, char const *rtspURL,
                                    int verbosityLevel = 0,
                                    char const *applicationName = NULL,
                                    portNumBits tunnelOverHTTPPortNum = 0, CQueue *queue = NULL,
                                    bool bTcp = false,
                                    unsigned udpReorderingHoldTime = 200,
                                    IRtspClientDestoryNotifyer *notify = nullptr);

protected:
    ourRTSPClient(UsageEnvironment &env, char const *rtspURL,
                  int verbosityLevel, char const *applicationName,
                  portNumBits tunnelOverHTTPPortNum, CQueue *queue,
                  bool bTcp,
                  unsigned udpReorderingHoldTime, IRtspClientDestoryNotifyer *notify = nullptr);

    // called only by createNew();
    virtual ~ourRTSPClient();

public:
    StreamClientState scs;
    CQueue *mQueue = nullptr;
    bool mBTcp;
    unsigned mUdpReorderingHoldTime;
    IRtspClientDestoryNotifyer *mNotify = nullptr;
};

//char eventLoopWatchVariable = 0;
/*------------------------------canok----------*/
//int main(int argc, char** argv) {
void *live555_main(void *src) {
    // Begin by setting up our usage environment:
    TaskScheduler *scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);
    SrcLive555 *obj = (SrcLive555 *) src;
    const char *url = obj->getUrl();
#if 0 //想拉高优先级，没有权限
    int ret =0;
    pid_t pid =getpid();
    int curschdu = sched_getscheduler(pid);
    if(curschdu < 0 ) {
        ALOGE("schedu err %s\n", strerror(errno));
    }

    ALOGD("[%s%D]schedu befor %d",__FUNCTION__ ,__LINE__,curschdu);
    struct sched_param s_parm;
    s_parm.sched_priority = sched_get_priority_max(SCHED_RR)-2;
    ALOGD("schedu max %d min %d",sched_get_priority_max(SCHED_RR),sched_get_priority_min(SCHED_FIFO));
    ret = sched_setscheduler(pid, SCHED_RR, &s_parm);
    if(ret < 0)
    {
        ALOGD( "schedu err %s\n",strerror( errno));
    }

    curschdu = sched_getscheduler(pid);
    if(curschdu <0 )
    {
        ALOGD( "schedu err %s",strerror( errno));
    }
    ALOGD("schedu after %d",curschdu);
#endif

    ALOGD("[%s%d] live555 open url:[%s] ", __FUNCTION__, __LINE__, url);
    obj->openURL(*env, "can_app", url);

    // All subsequent activity takes place within the event loop:
    env->taskScheduler().doEventLoop(obj->getEventLoopWatchVariable());
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.
    // If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
    // and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
    // then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
    shutdownStream(obj->getOurRtspClient());

    env->reclaim();
    env = NULL;
    delete scheduler;
    scheduler = NULL;

    ALOGD("[%d%s] live555 thread exit", __LINE__, __FUNCTION__);
    return NULL;
}


SrcLive555::SrcLive555(char *workdir) {
    mWorkdir = strDup(workdir);
}

SrcLive555::~SrcLive555() {
    stop();
    delete[] mWorkdir;
    delete mQueue;
}

void SrcLive555::start(char *url, bool bTcp, unsigned udpReorderTimeUs) {
    eventLoopWatchVariable = 0;
    mBTcp = bTcp;
    mUdpReorderingHoldTime = udpReorderTimeUs;
    if (mUrl) {
        delete[] mUrl;
    }
    mUrl = strDup(url);
    ALOGD(" srcLive555 [%s%d] url:%s, workdir %s", __FUNCTION__, __LINE__, url, mWorkdir);

#if ENABLE_DUMPFILE//if you want to dump data, enable this
    char file_out[128]={0};
    sprintf(file_out,"%s/live555get.h264",workdir);
    gfp_out = fopen(file_out,"w+");
    if(gfp_out==NULL){
        ALOGE("[%s%d] fopen err:%s",__FUNCTION__ ,__LINE__,file_out);
    }
#endif
    int ret = 0;
    eventLoopWatchVariable = 0;
    if (mQueue == NULL) {
        mQueue = new CQueue(60, "live555_RECV"); //512K * 60 == 30MByte
    }
    if (mQueue == NULL) {
        ALOGE("new queue erro");
        return;
    }

    if (0 != (ret = pthread_create(&pliveThread, NULL, live555_main, (void *) this))) {
        ALOGE("[%s%d]craete sch erro: %s", __FUNCTION__, __LINE__, strerror(ret));
        return;
    }

//    pthread_detach(pliveThread);
}


CQueue *SrcLive555::getDataQueue() {
    return mQueue;
}

const char *SrcLive555::getUrl() const {
    return mUrl;
}

ourRTSPClient *SrcLive555::getOurRtspClient() {
    return mRtspClient;
}

char *SrcLive555::getEventLoopWatchVariable() {
    return &eventLoopWatchVariable;
}


#define ENABLE_DUMPFILE  0

/*----------------------------------------*/

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.
class DummySink : public MediaSink {
public:
    static DummySink *createNew(UsageEnvironment &env,
                                MediaSubsession &subsession, // identifies the kind of data that's being received
                                char const *streamId = NULL,
                                CQueue *queue = NULL); // identifies the stream itself (optional)

private:
    DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId,
              CQueue *queue);

    // called only by "createNew()"
    virtual ~DummySink();

    static void afterGettingFrame(void *clientData, unsigned frameSize,
                                  unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned durationInMicroseconds);

    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                           struct timeval presentationTime, unsigned durationInMicroseconds);

private:
    // redefined virtual functions:
    virtual Boolean continuePlaying();

private:
    u_int8_t *fReceiveBuffer;
    MediaSubsession &fSubsession;
    char *fStreamId;
    CQueue *mQueue = nullptr;
};

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

//static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.

void SrcLive555::openURL(UsageEnvironment &env, char const *progName, char const *rtspURL) {
    // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
    // to receive (even if more than stream uses the same "rtsp://" URL).

    mRtspClient = ourRTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL,
                                           progName, 0, mQueue, mBTcp, mUdpReorderingHoldTime,
                                           this);

    if (mRtspClient == NULL) {
        env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": "
            << env.getResultMsg() << "\n";
        ALOGD("Failed to create a RTSP client for URL:%s\%s", rtspURL, env.getResultMsg());
        return;
    }

    mRtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}

void SrcLive555::onRtspClinetDestoryed(void *pram) {
    ALOGD("[%s%d] client had been destoryed!", __FUNCTION__, __LINE__);
    if (mRtspClient) {
        mRtspClient = nullptr;
    }
}

void SrcLive555::stop() {
    eventLoopWatchVariable = 1;
    pthread_join(pliveThread, NULL);
    if (mUrl) {
        delete[] mUrl;
        mUrl = nullptr;
    }
}
// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString) {
    do {
        UsageEnvironment &env = rtspClient->envir(); // alias
        StreamClientState &scs = ((ourRTSPClient *) rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
            ALOGE(" Failed to get a SDP description: %s", resultString);
            delete[] resultString;
            break;
        }

        char *const sdpDescription = resultString;
        env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";
        ALOGD("Got a SDP description: %s", sdpDescription);
        // Create a media session object from this SDP description:
        scs.session = MediaSession::createNew(env, sdpDescription);
        delete[] sdpDescription; // because we don't need it anymore
        if (scs.session == NULL) {
            env << *rtspClient
                << "Failed to create a MediaSession object from the SDP description: "
                << env.getResultMsg() << "\n";
            break;
        } else if (!scs.session->hasSubsessions()) {
            env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
            break;
        }

        // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
        // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
        // (Each 'subsession' will have its own data source.)
        scs.iter = new MediaSubsessionIterator(*scs.session);
        setupNextSubsession(rtspClient);
        return;
    } while (0);


    ALOGE("[%d%s] An unrecoverable error occurred with this stream shutdown starem ", __LINE__,
          __FUNCTION__);
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
//#define REQUEST_STREAMING_OVER_TCP True

void setupNextSubsession(RTSPClient *rtspClient) {
    UsageEnvironment &env = rtspClient->envir(); // alias
    StreamClientState &scs = ((ourRTSPClient *) rtspClient)->scs; // alias

    scs.subsession = scs.iter->next();
    if (scs.subsession != NULL) {
        if (!scs.subsession->initiate()) {
            env << *rtspClient << "Failed to initiate the \"" << *scs.subsession
                << "\" subsession: " << env.getResultMsg() << "\n";
            setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
        } else {
            env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
            if (scs.subsession->rtcpIsMuxed()) {
                env << "client port " << scs.subsession->clientPortNum();
            } else {
                env << "client ports " << scs.subsession->clientPortNum() << "-"
                    << scs.subsession->clientPortNum() + 1;
            }
            env << ")\n";

#if 1
            if (scs.subsession->rtpSource() != NULL) {
                int fd = scs.subsession->rtpSource()->RTPgs()->socketNum();

                int socket_buflen = 0;
                socklen_t result_len = sizeof(socket_buflen);

                if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &socket_buflen, &result_len) <
                    0) {
                    ALOGE("socket_recvBUf err %s\n", strerror(errno));
                }
                ALOGD("socket_recvBUf len %d\n", socket_buflen);

                //拉到最大值
                increaseReceiveBufferTo(env, fd, 2000 * 1024);

                if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &socket_buflen, &result_len) <
                    0) {
                    ALOGE("socket_recvBUf err %s\n", strerror(errno));
                }
                ALOGD("socket_recvBUf len %d\n", socket_buflen);
                ALOGD("socket_recvBUf len %d\n", getReceiveBufferSize(env, fd));
                /* Increase the RTP reorder timebuffer just a bit */
                // scs.subsession->rtpSource()->setPacketReorderingThresholdTime(1000);
                scs.subsession->rtpSource()->setPacketReorderingThresholdTime(
                        ((ourRTSPClient *) rtspClient)->mUdpReorderingHoldTime);
            }
#endif
            // Continue setting up this subsession, by sending a RTSP "SETUP" command:
            // rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
            rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False,
                                         ((ourRTSPClient *) rtspClient)->mBTcp);
        }
        return;
    }

    // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
    if (scs.session->absStartTime() != NULL) {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(),
                                    scs.session->absEndTime());
    } else {
        scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
    }
}

void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString) {
    do {
        UsageEnvironment &env = rtspClient->envir(); // alias
        StreamClientState &scs = ((ourRTSPClient *) rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: "
                << resultString << "\n";
            break;
        }

        env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
        if (scs.subsession->rtcpIsMuxed()) {
            env << "client port " << scs.subsession->clientPortNum();
        } else {
            env << "client ports " << scs.subsession->clientPortNum() << "-"
                << scs.subsession->clientPortNum() + 1;
        }
        env << ")\n";

        // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
        // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
        // after we've sent a RTSP "PLAY" command.)

        CQueue *queue = ((ourRTSPClient *) rtspClient)->mQueue;
        ALOGD("createsink :%d %p", __LINE__, queue);
        scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url(), queue);
        // perhaps use your own custom "MediaSink" subclass instead
        if (scs.subsession->sink == NULL) {
            env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
                << "\" subsession: " << env.getResultMsg() << "\n";
            break;
        }

        env << *rtspClient << "Created a data sink for the \"" << *scs.subsession
            << "\" subsession\n";
        scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
                                           subsessionAfterPlaying, scs.subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != NULL) {
            scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
        }
    } while (0);
    delete[] resultString;

    // Set up the next subsession, if any:
    setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString) {
    Boolean success = False;

    do {
        UsageEnvironment &env = rtspClient->envir(); // alias
        StreamClientState &scs = ((ourRTSPClient *) rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
            break;
        }
#if 0
        // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
        // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
        // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
        // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
        if (scs.duration > 0) {
            unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
            scs.duration += delaySlop;
            unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
            scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
        }
#endif
        env << *rtspClient << "Started playing session";
        if (scs.duration > 0) {
            env << " (for up to " << scs.duration << " seconds)";
        }
        env << "...\n";

        success = True;
    } while (0);
    delete[] resultString;

    if (!success) {
        // An unrecoverable error occurred with this stream.
        ALOGD("[%d%s]An unrecoverable error occurred with this stream, shutdown starem ", __LINE__,
              __FUNCTION__);
        shutdownStream(rtspClient);
    }
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void *clientData) {
    MediaSubsession *subsession = (MediaSubsession *) clientData;
    RTSPClient *rtspClient = (RTSPClient *) (subsession->miscPtr);

    // Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = NULL;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession &session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != NULL) {
        if (subsession->sink != NULL) return; // this subsession is still active
    }

    // All subsessions' streams have now been closed, so shutdown the client:
    ALOGD("[%d%s] shutdown starem ", __LINE__, __FUNCTION__);
    shutdownStream(rtspClient);
}

void subsessionByeHandler(void *clientData) {
    MediaSubsession *subsession = (MediaSubsession *) clientData;
    RTSPClient *rtspClient = (RTSPClient *) subsession->miscPtr;
    UsageEnvironment &env = rtspClient->envir(); // alias

    env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

    // Now act as if the subsession had closed:
    subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void *clientData) {
    ourRTSPClient *rtspClient = (ourRTSPClient *) clientData;
    StreamClientState &scs = rtspClient->scs; // alias

    scs.streamTimerTask = NULL;

    // Shut down the stream:
    ALOGE("[%s%d] timerout, shutdown stream ", __FUNCTION__, __LINE__);
    shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient *rtspClient, int exitCode) {
    if (!rtspClient) {
        return;
    }
    ALOGD("[%d%s] shutdown starem ", __LINE__, __FUNCTION__);
    UsageEnvironment &env = rtspClient->envir(); // alias
    StreamClientState &scs = ((ourRTSPClient *) rtspClient)->scs; // alias

    // First, check whether any subsessions have still to be closed:
    if (scs.session != NULL) {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*scs.session);
        MediaSubsession *subsession;

        while ((subsession = iter.next()) != NULL) {
            if (subsession->sink != NULL) {
                Medium::close(subsession->sink);
                subsession->sink = NULL;

                if (subsession->rtcpInstance() != NULL) {
                    subsession->rtcpInstance()->setByeHandler(NULL,
                                                              NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                }

                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive) {
            // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
            // Don't bother handling the response to the "TEARDOWN".
            rtspClient->sendTeardownCommand(*scs.session, NULL);
        }
    }

    env << *rtspClient << "Closing the stream.\n";
    ALOGD("Closing the stream ");
    //this operation will delete rtspClient
    Medium::close(rtspClient);
    ALOGD("Closing the stream over");
}


// Implementation of "ourRTSPClient":

ourRTSPClient *ourRTSPClient::createNew(UsageEnvironment &env, char const *rtspURL,
                                        int verbosityLevel, char const *applicationName,
                                        portNumBits tunnelOverHTTPPortNum, CQueue *queue,
                                        bool bTcp,
                                        unsigned udpReorderingHoldTime,
                                        IRtspClientDestoryNotifyer *notify) {
    return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum,
                             queue,
                             bTcp,
                             udpReorderingHoldTime, notify);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment &env, char const *rtspURL,
                             int verbosityLevel, char const *applicationName,
                             portNumBits tunnelOverHTTPPortNum, CQueue *queue,
                             bool bTcp,
                             unsigned udpReorderingHoldTime, IRtspClientDestoryNotifyer *notify)
        : RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1),
          mQueue(queue), mNotify(notify) {
}

ourRTSPClient::~ourRTSPClient() {
    ALOGD("[%d%s]", __LINE__, __FUNCTION__);
    if (mNotify) {
        mNotify->onRtspClinetDestoryed(nullptr);
    }
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
        : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
    delete iter;
    if (session != NULL) {
        // We also need to delete "session", and unschedule "streamTimerTask" (if set)
        UsageEnvironment &env = session->envir(); // alias

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}


// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 1024*512

DummySink *
DummySink::createNew(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId,
                     CQueue *queue) {
    return new DummySink(env, subsession, streamId, queue);
}

DummySink::DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId,
                     CQueue *queue)
        : MediaSink(env), mQueue(queue),
          fSubsession(subsession) {
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

DummySink::~DummySink() {
    delete[] fReceiveBuffer;
    delete[] fStreamId;
}

void DummySink::afterGettingFrame(void *clientData, unsigned frameSize, unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned durationInMicroseconds) {
    DummySink *sink = (DummySink *) clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 0

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned /*durationInMicroseconds*/) {
    // We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
    if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
    envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived "
            << frameSize << " bytes";
    if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
    char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
    sprintf(uSecsStr, "%06u", (unsigned) presentationTime.tv_usec);
    envir() << ".\tPresentation time: " << (int) presentationTime.tv_sec << "." << uSecsStr;
    if (fSubsession.rtpSource() != NULL &&
        !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
        envir()
                << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
    }
#ifdef DEBUG_PRINT_NPT
    envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
    envir() << "\n";
#endif

    //to do something
    if (mQueue) {
        char type = fReceiveBuffer[0] & ((1 << 5) - 1); //过滤掉 结束符, 某些h264流中有 00 00 01 09 ，不带数据
        if (type != 9) {
            // ALOGD("[%s%d] push one frame \n", __FUNCTION__, __LINE__);
            mQueue->push(fReceiveBuffer, frameSize);
//            if (gfp_out) {
//                //这里组包出来的h264帧，没有naul　头
//                unsigned char nal[4] = {0x00, 0x00, 0x00, 0x01};
//                fwrite(nal, 1, sizeof(nal), gfp_out);
//                fwrite(fReceiveBuffer, 1, frameSize, gfp_out);
//            }
        } else {
            ALOGD("END flag ,not push");
        }

    }
    // Then continue, to request the next frame of data:
    continuePlaying();
}

Boolean DummySink::continuePlaying() {
    if (fSource == NULL) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                          afterGettingFrame, this,
                          onSourceClosure, this);

    return True;
}
