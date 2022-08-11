LOCAL_PATH := $(call my-dir)

LIVE555DIR = ./live555
include $(CLEAR_VARS)
LOCAL_MODULE    := live_BasicUsageEnvironment
LOCAL_SRC_FILES :=  ./$(LIVE555DIR)/$(TARGET_ARCH_ABI)/usr/local/lib/libBasicUsageEnvironment.a
MY_ABSULATION_DIR=$(LOCAL_PATH)/$(LIVE555DIR)
LOCAL_EXPORT_C_INCLUDES := $(MY_ABSULATION_DIR)/  \
    $(MY_ABSULATION_DIR)/$(TARGET_ARCH_ABI)/usr/local/include \
    $(MY_ABSULATION_DIR)/$(TARGET_ARCH_ABI)/usr/local/include/BasicUsageEnvironment \
    $(MY_ABSULATION_DIR)/$(TARGET_ARCH_ABI)/usr/local/include/groupsock \
    $(MY_ABSULATION_DIR)/$(TARGET_ARCH_ABI)/usr/local/include/liveMedia \
    $(MY_ABSULATION_DIR)/$(TARGET_ARCH_ABI)/usr/local/include/UsageEnvironment
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := live_groupsock
LOCAL_SRC_FILES :=  ./$(LIVE555DIR)/$(TARGET_ARCH_ABI)/usr/local/lib/libgroupsock.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := live_liveMedia
LOCAL_SRC_FILES :=  ./$(LIVE555DIR)/$(TARGET_ARCH_ABI)/usr/local/lib/libliveMedia.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := live_UsageEnvironment
LOCAL_SRC_FILES :=  ./$(LIVE555DIR)/$(TARGET_ARCH_ABI)/usr/local/lib/libUsageEnvironment.a
include $(PREBUILT_STATIC_LIBRARY)


OBOE_DIR = ./oboe-1.6.1/prefab/modules/oboe
include $(CLEAR_VARS)
LOCAL_MODULE    := liboboe
LOCAL_SRC_FILES :=  ./$(OBOE_DIR)/libs/android.$(TARGET_ARCH_ABI)/liboboe.so
MY_ABSULATION_DIR=$(LOCAL_PATH)/$(OBOE_DIR)
LOCAL_EXPORT_C_INCLUDES := $(MY_ABSULATION_DIR)/include  \
    $(MY_ABSULATION_DIR)/include/oboe
include $(PREBUILT_SHARED_LIBRARY)



include $(CLEAR_VARS)
LOCAL_MODULE    := libbase
LOCAL_SRC_FILES :=  base/CMessage.cpp
LOCAL_SRC_FILES +=  base/CHandler.cpp
LOCAL_SRC_FILES +=  base/CLooper.cpp
LOCAL_SRC_FILES +=  base/CLooperRoster.cpp

LOCAL_SRC_FILES +=  base/utils/MediaBuffer.cpp
LOCAL_SRC_FILES +=  base/utils/MediaQueue.cpp
LOCAL_SRC_FILES +=  base/utils/queue.cpp
LOCAL_SRC_FILES +=  base/utils/timeUtils.cpp
LOCAL_CPP_FEATURES := rtti
LOCAL_APP_STL := c++_shared
LOCAL_CFLAGS += -Wall -Werror
LOCAL_LDLIBS := -llog -landroid

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/base
include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE    := baseTest
#LOCAL_SRC_FILES :=  base/test/LooperTest.cpp
#LOCAL_CFLAGS += -Wall -Werror
#LOCAL_LDLIBS := -llog -landroid
#LOCAL_STATIC_LIBRARIES := libbase
#include $(BUILD_EXECUTABLE)
##include $(BUILD_STATIC_LIBRARY)
##include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE    := live555player
LOCAL_SRC_FILES := live555player_jni.cpp
LOCAL_SRC_FILES += live555/live555.cpp
LOCAL_SRC_FILES += decoder/aacUtils.cpp
LOCAL_SRC_FILES += decoder/H264Decoder.cpp
LOCAL_SRC_FILES += decoder/AACDecoder.cpp
LOCAL_SRC_FILES += decoder/H265Decoder.cpp
LOCAL_SRC_FILES += Render/OboeRender.cpp

LOCAL_CFLAGS += -DNO_OPENSSL=1 -Wall -Werror
LOCAL_STATIC_LIBRARIES := live_liveMedia live_groupsock live_UsageEnvironment live_BasicUsageEnvironment
LOCAL_SHARED_LIBRARIES := liboboe libbase
LOCAL_LDLIBS :=  -lmediandk -llog -landroid
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := live555Server
LOCAL_SRC_FILES := live555Server_jni.cpp
LOCAL_SRC_FILES += encoder/H264Encoder.cpp
LOCAL_CFLAGS += -DNO_OPENSSL=1 -Wall -Werror
LOCAL_STATIC_LIBRARIES := live_liveMedia live_groupsock live_UsageEnvironment live_BasicUsageEnvironment
LOCAL_SHARED_LIBRARIES := libbase
LOCAL_LDLIBS :=  -lmediandk -llog -landroid
include $(BUILD_SHARED_LIBRARY)