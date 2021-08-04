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

include $(CLEAR_VARS)
LOCAL_MODULE    := live555player
LOCAL_SRC_FILES :=  live555player_jni.cpp
LOCAL_SRC_FILES += queue.cpp
LOCAL_SRC_FILES += live555/live555.cpp
LOCAL_SRC_FILES += mediacodec/Decoder.cpp
LOCAL_CFLAGS += -DNO_OPENSSL=1
LOCAL_STATIC_LIBRARIES := live_liveMedia live_groupsock live_UsageEnvironment live_BasicUsageEnvironment
#LOCAL_LDLIBS :=  -lmediandk -llog -landroid  -static-libstdc++　＃报错，libstdc++.a　里面又有未定义的符号
#LOCAL_LDLIBS :=  -lmediandk -llog -landroid -static-libc++ #报错，不支持这个参数
#LOCAL_LDLIBS :=  -lmediandk -llog -landroid -lc++ #运行起来后　无法加载libc++.so
#LOCAL_LDLIBS :=  -Wl,-Bdynamic -lmediandk -llog -landroid  -Wl,-Bstatic -lc++ -lc  -Wl,/data/nishome/td/xiancan.wang/Android/Sdk/ndk/21.1.6352462/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/21/crtbegin_static.o
#LOCAL_LDLIBS :=  -Wl,-Bdynamic -lmediandk -llog -landroid
#LOCAL_LDLIBS +=  -Wl,-Bstatic -lc++ -lc -lcompiler_rt-extras -ldl -lm -lz
LOCAL_LDLIBS :=  -lmediandk -llog -landroid
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)