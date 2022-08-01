//
// Created by canok on 2021/8/1.
//

#ifndef LIVE555_LOGS_H
#define LIVE555_LOGS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <android/log.h>
#ifndef LOG_TAG
#define LOG_TAG    __FILE__
#endif



#ifndef LOG_ALWAYS_FATAL_IF
#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))
#define LOG_ALWAYS_FATAL_IF(cond,...) \
     ( (CONDITION(cond)) \
     ? ((void)__android_log_assert(#cond, LOG_TAG, ## __VA_ARGS__)) \
     : (void)0 )


#ifndef LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL(...) \
    ( ((void)__android_log_assert(NULL, LOG_TAG, ## __VA_ARGS__)) )
#endif
#endif

#ifndef LOG_FATAL_IF
#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))
#define LOG_FATAL_IF(cond,...) \
     ( (CONDITION(cond)) \
     ? ((void)__android_log_assert(#cond, LOG_TAG, ## __VA_ARGS__)) \
     : (void)0 )
#endif

#define LITERAL_TO_STRING_INTERNAL(x)    #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)


#ifndef CHECK_OP

#define CHECK(condition)                                \
    LOG_ALWAYS_FATAL_IF(                                \
            !(condition),                               \
            "%s",                                       \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)    \
            " CHECK(" #condition ") failed.")
#endif

#define CHECK_OP(x,y,suffix,op)                                         \
    do {                                                                \
        const auto &a = x;                                              \
        const auto &b = y;                                              \
        if (!(a op b)) {                                                \
            std::string ___full =                                           \
                __FILE__ ":" LITERAL_TO_STRING(__LINE__)                \
                    " CHECK_" #suffix "( " #x "," #y ") failed: ";      \
            ___full.append(a);                                          \
            ___full.append(" vs. ");                                    \
            ___full.append(b);                                          \
            LOG_ALWAYS_FATAL("%s", ___full.c_str());                    \
        }                                                               \
    } while (false)

#define CHECK_EQ(x,y)   CHECK_OP(x,y,EQ,==)
#define CHECK_NE(x,y)   CHECK_OP(x,y,NE,!=)
#define CHECK_LE(x,y)   CHECK_OP(x,y,LE,<=)
#define CHECK_LT(x,y)   CHECK_OP(x,y,LT,<)
#define CHECK_GE(x,y)   CHECK_OP(x,y,GE,>=)
#define CHECK_GT(x,y)   CHECK_OP(x,y,GT,>)

#define TRESPASS(...) \
        LOG_ALWAYS_FATAL(                                       \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)            \
                " Should not be here. " __VA_ARGS__);

//#define TEST_DEBUG 1

#ifdef TEST_DEBUG
#define ALOGI(x...) printf(LOG_TAG);printf(x);printf("\n")
#define ALOGD(x...)  printf(LOG_TAG);printf(x);printf("\n")
#define ALOGW(x...)  printf(LOG_TAG);printf(x);printf("\n")
#define ALOGE(x...)  printf(LOG_TAG);printf(x);printf("\n")
#define ALOGF(x...) printf(LOG_TAG);printf(x);printf("\n")
#else
//#define LOG_TAG    "JNILOG" // 这个是自定义的LOG的标识
#define ALOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG, __VA_ARGS__)
#define ALOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG, __VA_ARGS__)
#define ALOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG, __VA_ARGS__)
#define ALOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG, __VA_ARGS__)
#define ALOGF(...)  __android_log_print(ANDROID_LOG_FATAL,LOG_TAG, __VA_ARGS__)
#endif
#ifdef __cplusplus
}
#endif

#endif //LIVE555_LOGS_H
