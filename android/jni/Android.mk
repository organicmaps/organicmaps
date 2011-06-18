LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := mapswithme
LOCAL_SRC_FILES := main_native.cpp
LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)
