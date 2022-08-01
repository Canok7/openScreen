//
// Created by Administrator on 2022/7/31.
//

#ifndef LIVE555_CLOOPERROSTER_H
#define LIVE555_CLOOPERROSTER_H

#include "CLooper.h"
#include <map>

class CLooperRoster {
public:
    CLooperRoster() = default;

    ~CLooperRoster() = default;

    CLooper::handler_id registerHandler(
            const std::shared_ptr<CLooper> &looper, const std::shared_ptr<CHandler> &handler);

    void unregisterHandler(CLooper::handler_id handlerID);

    void unregisterStaleHandlers();

    //void dump(int fd, const Vector<String16>& args);

private:
    struct HandlerInfo {
        std::weak_ptr<CLooper> mLooper;
        std::weak_ptr<CHandler> mHandler;
    };

    std::mutex mLock;;
    std::map<CLooper::handler_id, HandlerInfo> mHandlers;
    CLooper::handler_id mNextHandlerID = 1;
};


#endif //LIVE555_CLOOPERROSTER_H
