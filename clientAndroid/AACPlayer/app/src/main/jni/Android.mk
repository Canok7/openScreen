LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE    := aacDecoder
LOCAL_SRC_FILES :=  AudioTrackRender_jni.cpp
LOCAL_SRC_FILES += getAACFrame.cpp
LOCAL_SRC_FILES += Utils.cpp
LOCAL_LDLIBS := -lmediandk -llog
include $(BUILD_SHARED_LIBRARY)