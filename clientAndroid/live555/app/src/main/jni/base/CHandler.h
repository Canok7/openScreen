//
// Created by Administrator on 2022/7/31.
//

#ifndef LIVE555_CHANDLER_H
#define LIVE555_CHANDLER_H


#include <memory>
#include <map>
#include "CLooper.h"

class CMessage;

class CHandler :public std::enable_shared_from_this<CHandler> {
    friend class CMessage;      // deliverMessage()
    friend class CLooperRoster; // clear()
public:
    CLooper::handler_id id() const {
        return mID;
    }

    std::shared_ptr<CLooper> looper() const {
        return mLooper.lock();
    }

    std::weak_ptr<CLooper> getLooper() {
        return mLooper;
    }

    std::weak_ptr<CHandler> getHandler() {
        return shared_from_this();
    }

protected:
    virtual void onMessageReceived(const std::shared_ptr<CMessage> &msg) = 0;

private:

    CLooper::handler_id mID = 0;
    std::weak_ptr<CLooper> mLooper;

    inline void setID(CLooper::handler_id id, const std::weak_ptr<CLooper> &looper) {
        mID = id;
        mLooper = looper;
    }

    inline void clear() {
        mID = 0;
        mLooper.reset();
    }

    uint32_t mMessageCounter = 0;
    std::map<uint32_t, uint32_t> mMessages;

    void deliverMessage(const std::shared_ptr<CMessage> &msg);
};


#endif //LIVE555_CHANDLER_H
