//
// Created by Administrator on 2022/7/30.
//

#ifndef LIVE555_CLOOPER_H
#define LIVE555_CLOOPER_H

#include "Errors.h"
#include <string>
#include <memory>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>


typedef int64_t nsecs_t;       // nano-seconds
enum {
    SYSTEM_TIME_REALTIME = 0,  // system-wide realtime clock
    SYSTEM_TIME_MONOTONIC = 1, // monotonic time since unspecified starting point
    SYSTEM_TIME_PROCESS = 2,   // high-resolution per-process clock
    SYSTEM_TIME_THREAD = 3,    // high-resolution per-thread clock
    SYSTEM_TIME_BOOTTIME = 4   // same as SYSTEM_TIME_MONOTONIC, but including CPU suspend time
};

nsecs_t systemTime(int clock = SYSTEM_TIME_MONOTONIC);


class CHandler;

class CMessage;

class CReplyToken;

class CLooper : public std::enable_shared_from_this<CLooper> {
    // 友元可以访问 本类的private
    // 友元的申明 , 不受 pulibc, private的影响，它不是成员
    friend class CMessage;       // post()
public:
    typedef int32_t event_id;
    typedef int32_t handler_id;

    CLooper();

    ~CLooper();

    void setName(const std::string &name);

    std::string getName() {
        return mName;
    }

    handler_id registerHandler(const std::shared_ptr<CHandler> &handler);

    status_t
    start(bool runOnCallingThread = false, int32_t priority = 0/*do not support set for now*/);

    status_t stop();

    static int64_t GetNowUs();


private:

    bool loop();

    void post(const std::shared_ptr<CMessage> &msg, int64_t delayUs);

    // creates a reply token to be used with this looper
    std::shared_ptr<CReplyToken> createReplyToken();

    // waits for a response for the reply token.  If status is OK, the response
    // is stored into the supplied variable.  Otherwise, it is unchanged.
    status_t awaitResponse(const std::shared_ptr<CReplyToken> &replyToken,
                           std::shared_ptr<CMessage> *response);

    // posts a reply for a reply token.  If the reply could be successfully posted,
    // it returns OK. Otherwise, it returns an error value.
    status_t
    postReply(const std::shared_ptr<CReplyToken> &replyToken, const std::shared_ptr<CMessage> &msg);

private:
    struct Event {
        int64_t mWhenUs;
        std::shared_ptr<CMessage> mMessage;
    };
    std::string mName;
    //    https://blog.csdn.net/qq_44443986/article/details/115864344
    // 优先考虑 list,而不是depqueue
    std::list<Event> mEventQueue;

    std::mutex mLock;
    std::condition_variable mQueueChangedCondition;
    bool mRunningLocally = false;// 是否在start 调用的线程体中运行looper，而不是再启动一个新的线程

    std::mutex mRepliesLock;
    std::condition_variable mRepliesCondition;

    class LooperThreader : public std::enable_shared_from_this<LooperThreader> {
    public:
        LooperThreader(CLooper *looper);

        ~LooperThreader();

        status_t run(const std::string &name);

        void requestExit();

        // Call requestExit() and wait until this object's thread exits.
        // BE VERY CAREFUL of deadlocks. In particular, it would be silly to call
        // this function from this object's thread. Will return WOULD_BLOCK in
        // that case.
        status_t requestExitAndWait();

        bool isCurrentThread() const {
            return mThreadId == pthread_self();
        }

    private:
        bool exitPending();

    private:
        CLooper *mLooper = nullptr;
        pthread_t mThreadId = -1;
        std::shared_ptr<std::thread> mThread = nullptr;

        // note that all accesses of mExitPending and mRunning need to hold mLock
        std::mutex mLock;
        volatile bool mExitPending = false;
        volatile bool mRunning = true;
        std::condition_variable mThreadExitedCondition;
    };

    std::shared_ptr<LooperThreader> mThread;
};


#endif //LIVE555_CLOOPER_H
