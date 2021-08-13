LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE    := aacDecoder
LOCAL_SRC_FILES :=  AudioTrackRender_jni.cpp
LOCAL_SRC_FILES += AACFileparser.cpp
LOCAL_SRC_FILES += Utils.cpp
LOCAL_SRC_FILES += JavaAudioTrackRender.cpp
LOCAL_SRC_FILES += AACMediacodecDecoder.cpp
LOCAL_LDLIBS := -lmediandk -llog
LOCAL_CFLAGS += -Wall -Werror -Wno-unused-variable -Wno-reorder
include $(BUILD_SHARED_LIBRARY)