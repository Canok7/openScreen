//
// Created by xiancan.wang on 8/10/21.
//

#ifndef AACPLAYER_LOGS_H
#define AACPLAYER_LOGS_H
#ifdef __cplusplus
extern "C" {
#endif
#include <android/log.h>
#define LOG_TAG    "JNILOG" // 这个是自定义的LOG的标识
#define ALOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG, __VA_ARGS__)
#define ALOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG, __VA_ARGS__)
#define ALOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG, __VA_ARGS__)
#define ALOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG, __VA_ARGS__)
#define ALOGF(...)  __android_log_print(ANDROID_LOG_FATAL,LOG_TAG, __VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif //AACPLAYER_LOGS_H
