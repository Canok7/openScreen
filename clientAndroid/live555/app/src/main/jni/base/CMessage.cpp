//
// Created by Administrator on 2022/7/23.
//

#define LOG_TAG "CMessage"

#include "utils/logs.h"
#include "CMessage.h"
#include "CHandler.h"

#include <sstream>
#include <typeinfo>

status_t CReplyToken::setReply(const std::shared_ptr<CMessage> &reply) {
    if (mReplied) {
        ALOGE("trying to post a duplicate reply");
        return -EBUSY;
    }
    CHECK(mReply == nullptr);
    mReply = reply;
    mReplied = true;
    return OK;
}

CMessage::CMessage()
        : mWhat(0),
          mNumItems(0) {
}

CMessage::CMessage(uint32_t what, const std::shared_ptr<CHandler> &handler)
        : mWhat(what),
          mNumItems(0) {
    setTarget(handler);
}

CMessage::~CMessage() {
    clear();
}

void CMessage::setWhat(uint32_t what) {
    mWhat = what;
}

uint32_t CMessage::what() const {
    return mWhat;
}

void CMessage::setTarget(const std::shared_ptr<CHandler> &handler) {
    if (handler == nullptr) {
        mTarget = 0;
        mHandler.reset();
        mLooper.reset();
    } else {
        mTarget = handler->id();
        mHandler = handler->getHandler();
        mLooper = handler->getLooper();
    }
}

void CMessage::clear() {
    for (size_t i = 0; i < mNumItems; ++i) {
        Item *item = &mItems[i];
        delete[] item->mName;
        item->mName = nullptr;
        freeItemValue(item);
    }
    mNumItems = 0;
}

#define BASIC_TYPE(NAME, FIELDNAME, TYPENAME)                             \
void CMessage::set##NAME(const char *name, TYPENAME value) {            \
    Item *item = allocateItem(name);                                    \
                                                                        \
    item->mType = kType##NAME;                                          \
    item->u.FIELDNAME = value;                                          \
}                                                                       \
                                                                        \
/* NOLINT added to avoid incorrect warning/fix from clang.tidy */       \
bool CMessage::find##NAME(const char *name, TYPENAME *value) const {  /* NOLINT */ \
    const Item *item = findItem(name, kType##NAME);                     \
    if (item) {                                                         \
        *value = item->u.FIELDNAME;                                     \
        return true;                                                    \
    }                                                                   \
    return false;                                                       \
}

BASIC_TYPE(Int32, int32Value, int32_t)

BASIC_TYPE(Int64, int64Value, int64_t)

BASIC_TYPE(Size, sizeValue, size_t)

BASIC_TYPE(Float, floatValue, float)

BASIC_TYPE(Double, doubleValue, double)

BASIC_TYPE(Pointer, ptrValue, void *)

#undef BASIC_TYPE


void CMessage::setString(
        const char *name, const char *s, ssize_t len) {
    Item *item = allocateItem(name);
    item->mType = kTypeString;
    item->u.stringValue = new std::string(s, len < 0 ? strlen(s) : len);
}

void CMessage::setString(
        const char *name, const std::string &s) {
    setString(name, s.c_str(), s.size());
}

void CMessage::setMessage(const char *name, const std::shared_ptr<CMessage> &obj) {
    Item *item = allocateItem(name);
    item->mType = kTypeMessage;
    item->u.message = new std::shared_ptr<CMessage>(obj);
}

void CMessage::setObj(const char *name, const std::any &obj) {
    Item *item = allocateItem(name);
    item->mType = kTypeObject;
    item->u.objValue = new std::any(obj);//  new std::shared_ptr<obj.type()>(obj);
}

void CMessage::setRect(
        const char *name,
        int32_t left, int32_t top, int32_t right, int32_t bottom) {
    Item *item = allocateItem(name);
    item->mType = kTypeRect;

    item->u.rectValue.mLeft = left;
    item->u.rectValue.mTop = top;
    item->u.rectValue.mRight = right;
    item->u.rectValue.mBottom = bottom;
}

bool CMessage::contains(const char *name) const {
    size_t i = findItemIndex(name, strlen(name));
    return i < mNumItems;
}

bool CMessage::findString(const char *name, std::string *value) const {
    const Item *item = findItem(name, kTypeString);
    if (item) {
        *value = *item->u.stringValue;
        return true;
    }
    return false;
}

bool CMessage::findObject(const char *name, std::any *obj) const {
    const Item *item = findItem(name, kTypeObject);
    if (item) {
        *obj = *item->u.objValue;
        return true;
    }
    return false;
}

//bool CMessage::findObject(const char *name, std::shared_ptr<std::any> *obj) const {
//    const Item *item = findItem(name, kTypeObject);
//    if (item) {
//        *obj = *item->u.objValue;
//        return true;
//    }
//    return false;
//}

bool CMessage::findRect(
        const char *name,
        int32_t *left, int32_t *top, int32_t *right, int32_t *bottom) const {
    const Item *item = findItem(name, kTypeRect);
    if (item == nullptr) {
        return false;
    }

    *left = item->u.rectValue.mLeft;
    *top = item->u.rectValue.mTop;
    *right = item->u.rectValue.mRight;
    *bottom = item->u.rectValue.mBottom;

    return true;
}

status_t CMessage::post(int64_t delayUs) {
    std::shared_ptr<CLooper> looper = mLooper.lock();
    if (looper == nullptr) {
        ALOGW("failed to post message as target looper for handler is gone.");
        return -ENOENT;
    }

    looper->post(shared_from_this(), delayUs);
    return OK;
}

status_t CMessage::postAndAwaitResponse(std::shared_ptr<CMessage> *response) {
    std::shared_ptr<CLooper> looper = mLooper.lock();
    if (looper == nullptr) {
        ALOGW("failed to post message as target looper for handler  is gone");
        return -ENOENT;
    }

    std::shared_ptr<CReplyToken> token = looper->createReplyToken();
    if (token == nullptr) {
        ALOGE("failed to create reply token");
        return -ENOMEM;
    }

    setObj("replyID", token);
    looper->post(shared_from_this(), 0 /* delayUs */);
    return looper->awaitResponse(token, response);
}

bool CMessage::senderAwaitsResponse(std::shared_ptr<CReplyToken> *replyToken) {
    std::any tmp;
    bool found = findObject("replyID", &tmp);

    if (!found) {
        return false;
    }

//    ALOGD("[%s%d] has vaule:%d type:%s",__FUNCTION__ ,__LINE__,tmp.has_value(),tmp.type().name());
//    ALOGD("[%s%d] %d ",__FUNCTION__ ,__LINE__, tmp.type() == typeid(std::shared_ptr<CReplyToken>) );
    *replyToken = std::any_cast<std::shared_ptr<CReplyToken>>(tmp);
    setObj("replyID", nullptr);
    // TODO: delete Object instead of setting it to nullptr

    return *replyToken != nullptr;
}


status_t CMessage::postReply(const std::shared_ptr<CReplyToken> &replyToken) {
    if (replyToken == nullptr) {
        ALOGW("failed to post reply to a nullptr token");
        return -ENOENT;
    }
    std::shared_ptr<CLooper> looper = replyToken->getLooper();
    if (looper == nullptr) {
        ALOGW("failed to post reply as target looper is gone.");
        return -ENOENT;
    }
    return looper->postReply(replyToken, shared_from_this());
}


std::shared_ptr<CMessage> CMessage::dup() const {
    std::shared_ptr<CMessage> msg = std::make_shared<CMessage>(mWhat, mHandler.lock());
    msg->mNumItems = mNumItems;

    for (size_t i = 0; i < mNumItems; ++i) {
        const Item *from = &mItems[i];
        Item *to = &msg->mItems[i];

        to->setName(from->mName, from->mNameLength);
        to->mType = from->mType;

        switch (from->mType) {
            case kTypeString: {
                to->u.stringValue =
                        new std::string(*from->u.stringValue);
                break;
            }

            case kTypeObject: {
                to->u.objValue = from->u.objValue;
                break;
            }

            case kTypeMessage: {
                std::shared_ptr<CMessage> copy =
                        static_cast<std::shared_ptr<CMessage>>((*(from->u.message))->dup());
                break;
            }

            default: {
                to->u = from->u;
                break;
            }
        }
    }

    return msg;
}

static bool isFourcc(uint32_t what) {
    return isprint(what & 0xff)
           && isprint((what >> 8) & 0xff)
           && isprint((what >> 16) & 0xff)
           && isprint((what >> 24) & 0xff);
}

static void appendIndent(std::string *s, int32_t indent) {
    static const char kWhitespace[] =
            "                                        "
            "                                        ";
//    CHECK_LT((size_t)indent, sizeof(kWhitespace));
    s->append(kWhitespace, indent);
}

std::string CMessage::toString() const {
    std::string s = "AMessage(what = ";

//    std::string tmp;
    std::stringstream tmp;
    if (isFourcc(mWhat)) {
        tmp << (char) (mWhat >> 24)
            << (char) ((mWhat >> 16) & 0xff)
            << (char) ((mWhat >> 8) & 0xff)
            << (char) (mWhat & 0xff);
    } else {
        tmp << mWhat;
    }
    s.append(tmp.str());

    if (mTarget != 0) {
        tmp.clear();
        tmp << ", target = " << mTarget << ",";
        s.append(tmp.str());
    }
    s.append(") = {\n");

    tmp.clear();
    for (size_t i = 0; i < mNumItems; ++i) {
        const Item &item = mItems[i];

        switch (item.mType) {
            case kTypeInt32:
                tmp << "int32_t " << item.mName << " = " << item.u.int32Value;
                break;
            case kTypeInt64:
                tmp << "int64_t" << item.mName << " = " << item.u.int64Value;
                break;
            case kTypeSize:
                tmp << "size_t" << item.mName << " = " << item.u.sizeValue;
                break;
            case kTypeFloat:
                tmp << "float" << item.mName << " = " << item.u.floatValue;
                break;
            case kTypeDouble:
                tmp << "double" << item.mName << " = " << item.u.doubleValue;
                break;
            case kTypePointer:
                tmp << "void" << item.mName << " = " << item.u.ptrValue;
                break;
            case kTypeString:
                tmp << "string" << item.mName << " = " << item.u.stringValue;
                break;
            case kTypeObject:
                tmp << "objValue" << item.mName << " = " << item.u.objValue;
                break;
            case kTypeMessage:
                tmp << "CMessage" << item.mName << " = " << item.u.message;
                break;
            case kTypeRect:
                tmp << "Rect" << item.mName << " = " << "(" << item.u.rectValue.mLeft << ", "
                    << item.u.rectValue.mTop << ", " << item.u.rectValue.mRight << ", "
                    << item.u.rectValue.mBottom;
                break;
            default:
                TRESPASS();
        }

        appendIndent(&s, 0);
        s.append("  ");
        s.append(tmp.str());
        s.append("\n");
    }

    appendIndent(&s, 0);
    s.append("}");

    return s;
}

CMessage::Item *CMessage::allocateItem(const char *name) {
    size_t len = strlen(name);
    size_t i = findItemIndex(name, len);
    Item *item;

    if (i < mNumItems) {
        item = &mItems[i];
        freeItemValue(item);
    } else {
        CHECK(mNumItems < kMaxNumItems);
        i = mNumItems++;
        item = &mItems[i];
        item->mType = kTypeInt32;
        item->setName(name, len);
    }

    return item;
}

void CMessage::freeItemValue(Item *item) {
    switch (item->mType) {
        case kTypeString: {
            delete item->u.stringValue;
            break;
        }

        case kTypeObject: {
//            if (item->u.objValue != nullptr) {
                delete item->u.objValue;
//            }
            break;
        }

        case kTypeMessage: {
//            if (item->u.message != nullptr) {
                delete item->u.message;
//            }
            break;
        }
        default:
            break;
    }
    item->mType = kTypeInt32; // clear type
}

const CMessage::Item *CMessage::findItem(
        const char *name, Type type) const {
    size_t i = findItemIndex(name, strlen(name));
    if (i < mNumItems) {
        const Item *item = &mItems[i];
        return item->mType == type ? item : nullptr;

    }
    return nullptr;
}

inline size_t CMessage::findItemIndex(const char *name, size_t len) const {

    size_t i = 0;
    for (; i < mNumItems; i++) {
        if (len != mItems[i].mNameLength) {
            continue;
        }

        if (!memcmp(mItems[i].mName, name, len)) {
            break;
        }
    }
    return i;
}

void CMessage::deliver() {
    std::shared_ptr<CHandler> handler = mHandler.lock();
    if (handler == nullptr) {
        ALOGW("failed to deliver message as target handler %d is gone.", mTarget);
        return;
    }

    handler->deliverMessage(shared_from_this());
}


void CMessage::Item::setName(const char *name, size_t len) {
    mNameLength = len;
    mName = new char[len + 1];
    memcpy((void *) mName, name, len + 1);
}