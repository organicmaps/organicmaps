LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#LOCAL_CPP_FEATURES += exceptions rtti

LOCAL_MODULE := mapswithme

LOCAL_CFLAGS := -ffunction-sections -fdata-sections

TARGET_PLATFORM := android-5

LOCAL_HEADER_FILES := \
 	com/mapswithme/core/jni_helper.hpp \
	com/mapswithme/core/jni_string.hpp \
	com/mapswithme/core/logging.hpp \
	com/mapswithme/core/render_context.hpp \
	com/mapswithme/maps/Framework.hpp \
	com/mapswithme/platform/Platform.hpp \
	com/mapswithme/platform/http_thread_android.hpp \

LOCAL_SRC_FILES := \
	com/mapswithme/core/jni_helper.cpp \
	com/mapswithme/core/jni_string.cpp \
	com/mapswithme/core/logging.cpp \
	com/mapswithme/core/render_context.cpp \
	com/mapswithme/maps/DownloadUI.cpp \
	com/mapswithme/maps/Framework.cpp \
	com/mapswithme/maps/VideoTimer.cpp \
	com/mapswithme/maps/MWMActivity.cpp \
	com/mapswithme/maps/SmartRenderer.cpp \
	com/mapswithme/maps/SmartGLSurfaceView.cpp \
	com/mapswithme/platform/Platform.cpp \
	com/mapswithme/platform/HttpThread.cpp \
	com/mapswithme/platform/Language.cpp \
	com/mapswithme/jni/jni_thread.cpp \
	com/mapswithme/jni/jni_method.cpp \

LOCAL_LDLIBS := -llog -lGLESv1_CM \
		-lmap -lversion -lsearch -lstorage -lindexer -lyg -lplatform \
		-lgeometry -lcoding -lbase -lexpat -lfreetype -lfribidi -lzlib -lbzip2 \
		-ljansson -ltomcrypt -lprotobuf ./obj/local/$(TARGET_ARCH_ABI)/libgnustl_static.a

LOCAL_LDLIBS += -Wl,--gc-sections

OMIM_CONFIG := release
ifeq ($(NDK_DEBUG),1)
  OMIM_CONFIG := debug
else
  ifeq ($(PRODUCTION),1)
    OMIM_CONFIG := production
  endif
  LOCAL_CFLAGS += -O3
endif

LOCAL_LDLIBS += -L../../omim-android-$(OMIM_CONFIG)-$(TARGET_ARCH_ABI)/out/$(OMIM_CONFIG)

include $(BUILD_SHARED_LIBRARY)
