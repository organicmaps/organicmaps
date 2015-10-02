
LOCAL_PATH := $(call my-dir)

# Add prebuilt shared lib
# todo(vbykoianko) extract the names of the libs from tests_list.sh
MY_PREBUILT_LIBS_PATH := ../libs/tmp/$(TARGET_ARCH_ABI)
$(info $(MY_PREBUILT_LIBS_PATH))

include $(CLEAR_VARS)
LOCAL_MODULE := routing_integration_tests
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/librouting_integration_tests.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := indexer_tests
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libindexer_tests.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

ROOT_PATH := ../..
include ./jni/AndroidBeginning.mk

LOCAL_MODULE    := all_tests

LOCAL_SRC_FILES := ./main.cpp
LOCAL_STATIC_LIBRARIES := android_native_app_glue

# todo(vbykoianko) extract the names of the libs from tests_list.sh
LOCAL_SHARED_LIBRARIES := routing_integration_tests indexer_tests 

include ./jni/AndroidEnding.mk
