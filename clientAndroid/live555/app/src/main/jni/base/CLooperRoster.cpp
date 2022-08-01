//
// Created by Administrator on 2022/7/31.
//

#include "logs.h"
#include "CLooperRoster.h"
#include "CLooper.h"
#include "CHandler.h"
#include <vector>

CLooper::handler_id CLooperRoster::registerHandler(
        const std::shared_ptr<CLooper> &looper, const std::shared_ptr<CHandler> &handler) {
    std::unique_lock<std::mutex> lk(mLock);
    if (handler->id() != 0) {
        CHECK(!"A handler must only be registered once.");
        return INVALID_OPERATION;
    }

    HandlerInfo info;
    info.mLooper = looper;
    info.mHandler = handler;
    CLooper::handler_id handlerID = mNextHandlerID++;
    mHandlers[handlerID] = info;

    handler->setID(handlerID, looper);

    return handlerID;
}

void CLooperRoster::unregisterHandler(CLooper::handler_id handlerID) {

    std::unique_lock<std::mutex> lk(mLock);
    if (mHandlers.count(handlerID) == 0) {
        return;
    }

    const HandlerInfo &info = mHandlers[handlerID];

    std::shared_ptr<CHandler> handler = info.mHandler.lock();

    if (handler != nullptr) {
        handler->clear();
    }

    mHandlers.erase(handlerID);

}

void CLooperRoster::unregisterStaleHandlers() {

    std::vector<std::shared_ptr<CLooper>> activeLoopers;
    {

        std::unique_lock<std::mutex> lk(mLock);

        std::map<CLooper::handler_id, HandlerInfo>::iterator it;
        for (it = mHandlers.begin(); it != mHandlers.end();) {
            std::shared_ptr<CLooper> looper = it->second.mLooper.lock();
            if (nullptr == looper) {
                mHandlers.erase(it);
            } else {
                activeLoopers.push_back(looper);
            }
            it++;
        }
    }
}

//void dump(int fd, const Vector<String16>& args);