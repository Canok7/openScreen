//
// Created by Administrator on 2022/7/23.
//

#ifndef LIVE555_CMESSAGE_H
#define LIVE555_CMESSAGE_H


#include <cstdint>
#include <memory>
#include <any>

#include "Errors.h"
#include "CLooper.h"

struct Rect {
    int32_t mLeft, mTop, mRight, mBottom;
};

class CHandler;

class CMessage;


class CReplyToken {
    friend CMessage;
    friend CLooper; // for retrieveReply
public:
    explicit CReplyToken(const std::shared_ptr<CLooper> &looper)
            : mLooper(looper),
              mReplied(false) {
    }

private:
    std::weak_ptr<CLooper> mLooper;
    std::shared_ptr<CMessage> mReply;
    bool mReplied;

    std::shared_ptr<CLooper> getLooper() const {
        return mLooper.lock();
    }

    // if reply is not set, returns false; otherwise, it retrieves the reply and returns true
    bool retrieveReply(std::shared_ptr<CMessage> *reply) {
        if (mReplied) {
            *reply = mReply;
            mReply.reset();
        }
        return mReplied;
    }

    // sets the reply for this token. returns OK or error
    status_t setReply(const std::shared_ptr<CMessage> &reply);
};

class CMessage : public std::enable_shared_from_this<CMessage> {
public:
    enum Type {
        kTypeInt32,
        kTypeInt64,
        kTypeSize,
        kTypeFloat,
        kTypeDouble,
        kTypePointer,
        kTypeString,
        kTypeObject,
        kTypeMessage,
        kTypeRect,
//        kTypeBuffer,
    };

    CMessage();

    CMessage(uint32_t what, const std::shared_ptr<CHandler> &handler);

    virtual ~CMessage();

    void setWhat(uint32_t);

    uint32_t what() const;

    void setTarget(const std::shared_ptr<CHandler> &handler);

    void clear();

    void setInt32(const char *name, int32_t value);

    void setInt64(const char *name, int64_t value);

    void setSize(const char *name, size_t value);

    void setFloat(const char *name, float value);

    void setDouble(const char *name, double value);

    void setPointer(const char *name, void *value);

    void setString(const char *name, const char *s, ssize_t len = -1);

    void setString(const char *name, const std::string &s);

    void setMessage(const char *name, const std::shared_ptr<CMessage> &obj);

//    void setAny(const char *name, const std::shared_ptr<std::any> &obj);
//    void setBuffer(const char *name, const sp<ABuffer> &buffer);
//    void setObj(const char *name, const std::shared_ptr<void> obj);
//    void setObj(const char *name, const std::shared_ptr<std::any> obj);
    void setObj(const char *name, const std::any &obj);

    void setRect(
            const char *name,
            int32_t left, int32_t top, int32_t right, int32_t bottom);

    bool contains(const char *name) const;

    bool findInt32(const char *name, int32_t *value) const;

    bool findInt64(const char *name, int64_t *value) const;

    bool findSize(const char *name, size_t *value) const;

    bool findFloat(const char *name, float *value) const;

    bool findDouble(const char *name, double *value) const;

    bool findPointer(const char *name, void **value) const;

    bool findString(const char *name, std::string *value) const;

    bool findObject(const char *name, std::any *obj) const;
//    bool findObject(const char *name, std::shared_ptr<std::any> *obj) const;

    bool findRect(
            const char *name,
            int32_t *left, int32_t *top, int32_t *right, int32_t *bottom) const;


    // ---------- used by sender
    status_t post(int64_t delayUs = 0);

    // Posts the message to its target and waits for a response (or error)
    // before returning.
    status_t postAndAwaitResponse(std::shared_ptr<CMessage> *response);


    // ---------- used by reciver
    // If this returns true, the sender of this message is synchronously
    // awaiting a response and the reply token is consumed from the message
    // and stored into replyID. The reply token must be used to send the response
    // using "postReply" below.
    bool senderAwaitsResponse(std::shared_ptr<CReplyToken> *replyID);

    // Posts the message as a response to a reply token.  A reply token can
    // only be used once. Returns OK if the response could be posted; otherwise,
    // an error.
    status_t postReply(const std::shared_ptr<CReplyToken> &replyID);

    // Performs a deep-copy of "this", contained messages are in turn "dup'ed".
    // Warning: RefBase items, i.e. "objects" are _not_ copied but only have
    // their refcount incremented.
    std::shared_ptr<CMessage> dup() const;

    std::string toString() const;

//    // Adds all items from other into this.
//    void extend(const std::shared_ptr<CMessage> &other);

private:
    void deliver();

private:
    friend class CLooper;

    uint32_t mWhat;
    std::weak_ptr<CHandler> mHandler;
    std::weak_ptr<CLooper> mLooper;

    // used only for debugging
    CLooper::handler_id mTarget = 0;

    class Item {
    public:
        union uData {
            // 不嵌套class
            int32_t int32Value;
            int64_t int64Value;
            size_t sizeValue;
            float floatValue;
            double doubleValue;
            std::shared_ptr<CMessage> *message;
//            CMessage *message;
            void *ptrValue;
//            RefBase *refValue;
//            std::shared_ptr<std::any> *objValue;
            std::any *objValue;
//            void *objValue;
            std::string *stringValue;
            Rect rectValue;
        } u;

        const char *mName;
        size_t mNameLength;
        Type mType;

        void setName(const char *name, size_t len);
    };

    enum {
        kMaxNumItems = 64
    };
    Item mItems[kMaxNumItems];
    size_t mNumItems = 0;

    Item *allocateItem(const char *name);

    void freeItemValue(Item *item);

    const Item *findItem(const char *name, Type type) const;

    size_t findItemIndex(const char *name, size_t len) const;

};


#endif //LIVE555_CMESSAGE_H
