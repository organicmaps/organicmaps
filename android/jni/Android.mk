LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := mapswithme

LOCAL_HEADER_FILES := \
	jni_helper.h \
	jni_string.h \
	logging.h \

LOCAL_SRC_FILES := \
	main_native.cpp \
	jni_helper.cpp \
	jni_string.cpp \
	platform.cpp \

LOCAL_LDLIBS := -llog \
  	-lwords -lmap -lstorage -lversion -lsearch -lindexer -lyg -lplatform \
  	-lgeometry -lcoding -lbase -lexpat -lfreetype -lfribidi -lzlib -lbzip2 \
  	-ljansson -ltomcrypt ./obj/local/armeabi/libstdc++.a

LOCAL_LDLIBS += -L../../omim-android-debug/out/debug
#LOCAL_LDLIBS += -L../../omim-android-release/out/release

include $(BUILD_SHARED_LIBRARY)
