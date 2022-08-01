//
// Created by Administrator on 2022/7/30.
//

#define LOG_TAG "CLooper"

#include "logs.h"
#include "CLooperRoster.h"
#include "CLooper.h"
#include "CHandler.h"
#include "CMessage.h"

nsecs_t systemTime(int clock) {
    static const clockid_t clocks[] = {
            CLOCK_REALTIME,
            CLOCK_MONOTONIC,
            CLOCK_PROCESS_CPUTIME_ID,
            CLOCK_THREAD_CPUTIME_ID,
            CLOCK_BOOTTIME
    };
    struct timespec t={0};
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(clocks[clock], &t);
    return nsecs_t(t.tv_sec) * 1000000000LL + t.tv_nsec;
}


// static
int64_t CLooper::GetNowUs() {
    return systemTime(SYSTEM_TIME_MONOTONIC) / 1000LL;
}

CLooperRoster gLooperRoster;

CLooper::CLooper() {
    // clean up stale AHandlers. Doing it here instead of in the destructor avoids
    // the side effect of objects being deleted from the unregister function recursively.
    gLooperRoster.unregisterStaleHandlers();
}

CLooper::~CLooper() {
    stop();
    // stale AHandlers are now cleaned up in the constructor of the next ALooper to come along
}

void CLooper::setName(const std::string &name) {
    mName = name;
}


CLooper::handler_id CLooper::registerHandler(const std::shared_ptr<CHandler> &handler) {
    return gLooperRoster.registerHandler(shared_from_this(), handler);
}

status_t CLooper::start(bool runOnCallingThread, int32_t priority) {
    (void)priority;
    if (runOnCallingThread) {
        mRunningLocally = true;
        ALOGD("now we do not support!");
        return INVALID_OPERATION;
    }

    std::unique_lock<std::mutex> lk(mLock);
    if (mThread != nullptr || mRunningLocally) {
        ALOGD("[%s%d] had been runed!", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    mThread = std::make_shared<LooperThreader>(this);

    status_t err = mThread->run(mName.empty() ? "Looper" : mName.c_str());
    if (err != OK) {
        mThread.reset();
    }

    return OK;
}

status_t CLooper::stop() {
    std::shared_ptr<LooperThreader> thread;
    bool runningLocally;
    {
        std::unique_lock<std::mutex> lk(mLock);
        thread = mThread;
        runningLocally = mRunningLocally;

        mThread.reset();
        mRunningLocally = false;
    }

    if (thread == nullptr && !runningLocally) {
        return INVALID_OPERATION;
    }

    if (thread != nullptr) {
        thread->requestExit();
    }

    mQueueChangedCondition.notify_one();
    {
        std::unique_lock<std::mutex> lk(mRepliesLock);
        mRepliesCondition.notify_all();
    }

    if (!runningLocally && !thread->isCurrentThread()) {
        // If not running locally and this thread _is_ the looper thread,
        // the loop() function will return and never be called again.
        thread->requestExitAndWait();
    }
    return OK;
}

bool CLooper::loop() {
    Event event;

    {
        std::unique_lock<std::mutex> lk(mLock);
        if (mThread == nullptr && !mRunningLocally) {
            return false;
        }
        if (mEventQueue.empty()) {
            mQueueChangedCondition.wait(lk);
            return true;
        }
        int64_t whenUs = mEventQueue.front().mWhenUs;
        int64_t nowUs = GetNowUs();

        if (whenUs > nowUs) {
            int64_t delayUs = whenUs - nowUs;
            if (delayUs > INT64_MAX / 1000) {
                delayUs = INT64_MAX / 1000;
            }

            mQueueChangedCondition.wait_for(lk, std::chrono::nanoseconds(delayUs * 1000ll));
            return true;
        }

        event = mEventQueue.front();
        mEventQueue.pop_front();
    }
    event.mMessage->deliver();

// NOTE: It's important to note that at this point our "ALooper" object
// may no longer exist (its final reference may have gone away while
// delivering the message). We have made sure, however, that loop()
// won't be called again.

    return true;
}


void CLooper::post(const std::shared_ptr<CMessage> &msg, int64_t delayUs) {
    std::unique_lock<std::mutex> lk(mLock);
    int64_t whenUs;
    if (delayUs > 0) {
        int64_t nowUs = GetNowUs();
        whenUs = (delayUs > INT64_MAX - nowUs ? INT64_MAX : nowUs + delayUs);

    } else {
        whenUs = GetNowUs();
    }

    //保持 有序地插入， list里面是按照时间排序的
    auto it = mEventQueue.begin();
    while (it != mEventQueue.end() && (*it).mWhenUs <= whenUs) {
        ++it;
    }

    Event event;
    event.mWhenUs = whenUs;
    event.mMessage = msg;

    if (it == mEventQueue.begin()) {
        mQueueChangedCondition.notify_one();
    }

    mEventQueue.insert(it, event);
}


std::shared_ptr<CReplyToken> CLooper::createReplyToken() {
    return std::make_shared<CReplyToken>(shared_from_this());
}

// to be called by CMessage::postAndAwaitResponse only
status_t CLooper::awaitResponse(const std::shared_ptr<CReplyToken> &replyToken,
                                std::shared_ptr<CMessage> *response) {
    // return status in case we want to handle an interrupted wait
    std::unique_lock<std::mutex> replyLock(mRepliesLock);
    CHECK(replyToken != nullptr);
    while (!replyToken->retrieveReply(response)) {
        {

            std::unique_lock<std::mutex> lk(mLock);
            if (mThread == nullptr) {
                return -ENOENT;
            }
        }
        mRepliesCondition.wait(replyLock);
    }
    return OK;
}

status_t CLooper::postReply(const std::shared_ptr<CReplyToken> &replyToken,
                            const std::shared_ptr<CMessage> &reply) {
    std::unique_lock<std::mutex> lkRepliyes(mRepliesLock);
    status_t err = replyToken->setReply(reply);
    if (err == OK) {
        mRepliesCondition.notify_all();
    }
    return err;
}

CLooper::LooperThreader::LooperThreader(CLooper *looper) : mLooper(looper) {
//    ALOGD("[%s%d] debug", __FUNCTION__, __LINE__);
}

CLooper::LooperThreader::~LooperThreader() {
    requestExitAndWait();
    mThread->join(); //
}

status_t CLooper::LooperThreader::run(const std::string &name) {
    auto thread_run = [=]() {
        mThreadId = pthread_self();
        pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
//        mThreadId = std::this_thread::get_id();
//        mThreadId = mThread->get_id();

        bool result = true;
        while (result && !exitPending()) {
            result = mLooper->loop();
        }

        // establish a scope for mLock
        {
            std::unique_lock<std::mutex> lk(mLock);
            if (result == false || mExitPending) {
                // clear thread ID so that requestExitAndWait() does not exit if
                // called by a new thread using the same thread ID as this one.

                mExitPending = true;
                mRunning = false;
                // clear thread ID so that requestExitAndWait() does not exit if
                // called by a new thread using the same thread ID as this one.
                mThreadId = pthread_t(-1);
                // note that interested observers blocked in requestExitAndWait are
                // awoken by broadcast, but blocked on mLock until break exits scope
                mThreadExitedCondition.notify_all();

            }
        }
    };

    mExitPending = false;
    mRunning = true;
    mThreadId = pthread_t(-1);
    mThread = std::make_shared<std::thread>(thread_run);
    return OK;
}

void CLooper::LooperThreader::requestExit() {
    std::unique_lock<std::mutex> lk(mLock);
    mExitPending = true;
}

status_t CLooper::LooperThreader::requestExitAndWait() {
    std::unique_lock<std::mutex> lk(mLock);
    if (isCurrentThread()) {//do not to wait self exit
        ALOGW(
                "Thread (this=%p): don't call waitForExit() from this "
                "Thread object's thread. It's a guaranteed deadlock!",
                this);

        return WOULD_BLOCK;
    }

    mExitPending = true;
    while (mRunning == true) {
        mThreadExitedCondition.wait(lk);
    }
    mExitPending = false;

    return OK;
}

bool CLooper::LooperThreader::exitPending() {
    std::unique_lock<std::mutex> lk(mLock);
    return mExitPending;
}