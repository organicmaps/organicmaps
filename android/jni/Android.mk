LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := mapswithme
LOCAL_SRC_FILES := main_native.cpp
LOCAL_LDLIBS    := -llog -L../../omim-android-release/out/release \
  	-lwords -lmap -lstorage -lversion -lsearch -lindexer -lyg -lplatform \
  	-lgeometry -lcoding -lbase -lexpat -lfreetype -lfribidi -lzlib -lbzip2 \
  	-ljansson -ltomcrypt

include $(BUILD_SHARED_LIBRARY)
