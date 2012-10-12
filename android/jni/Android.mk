LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#LOCAL_CPP_FEATURES += exceptions rtti

LOCAL_MODULE := mapswithme

LOCAL_CFLAGS := -ffunction-sections -fdata-sections

TARGET_PLATFORM := android-5

LOCAL_HEADER_FILES := \
 	com/mapswithme/core/jni_helper.hpp \
	com/mapswithme/core/logging.hpp \
	com/mapswithme/core/render_context.hpp \
	com/mapswithme/maps/Framework.hpp \
	com/mapswithme/platform/Platform.hpp \
	com/mapswithme/platform/http_thread_android.hpp \
	nv_thread/nv_thread.hpp \
	nv_event/nv_event_queue.hpp \
	nv_event/nv_event.hpp \
	nv_event/nv_keycode_mapping.hpp \
	nv_event/scoped_profiler.hpp

LOCAL_SRC_FILES := \
	com/mapswithme/core/jni_helper.cpp \
	com/mapswithme/core/logging.cpp \
	com/mapswithme/core/render_context.cpp \
	com/mapswithme/maps/Framework.cpp \
	com/mapswithme/maps/VideoTimer.cpp \
	com/mapswithme/maps/MWMActivity.cpp \
	com/mapswithme/maps/MWMApplication.cpp \
	com/mapswithme/maps/Lifecycle.cpp \
	com/mapswithme/maps/LocationState.cpp \
	com/mapswithme/maps/MapStorage.cpp \
	com/mapswithme/maps/DownloadResourcesActivity.cpp \
	com/mapswithme/maps/SearchActivity.cpp \
	com/mapswithme/maps/SettingsActivity.cpp \
	com/mapswithme/platform/Platform.cpp \
	com/mapswithme/platform/HttpThread.cpp \
	com/mapswithme/platform/Language.cpp \
  com/mapswithme/platform/PThreadImpl.cpp \
	nv_thread/nv_thread.cpp \
	nv_event/nv_event_queue.cpp \
	nv_event/nv_event.cpp \
	nv_time/nv_time.cpp

LOCAL_LDLIBS := -llog -lGLESv2 \
		-lmap -lgui -lversion -lsearch -lstorage -lindexer -lyg -lplatform -lanim \
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
