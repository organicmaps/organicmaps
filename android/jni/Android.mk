LOCAL_PATH := $(call my-dir)

OMIM_CONFIG := release
ifeq ($(NDK_DEBUG),1)
  OMIM_CONFIG := debug
else
  ifeq ($(PRODUCTION),1)
    OMIM_CONFIG := production
  endif
endif

####################################################################################
# List all static libraries which are built using our own scripts in tools/android #
####################################################################################

MY_PREBUILT_LIBS_PATH := ../../../omim-android-$(OMIM_CONFIG)-$(TARGET_ARCH_ABI)/out/$(OMIM_CONFIG)

include $(CLEAR_VARS)
LOCAL_MODULE := protobuf
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libprotobuf.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := tomcrypt
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libtomcrypt.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := jansson
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libjansson.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := bzip2
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libbzip2.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := zlib
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libzlib.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := fribidi
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libfribidi.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := freetype
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libfreetype.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := expat
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libexpat.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := base
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libbase.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := coding
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libcoding.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := geometry
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libgeometry.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := anim
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libanim.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := platform
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libplatform.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := graphics
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libgraphics.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := indexer
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libindexer.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := storage
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libstorage.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := search
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libsearch.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := gui
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libgui.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := map
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libmap.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := stats_client
LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/libstats_client.a
include $(PREBUILT_STATIC_LIBRARY)

########################### Main MapsWithMe module ############################

include $(CLEAR_VARS)

#LOCAL_CPP_FEATURES += exceptions rtti

LOCAL_MODULE := mapswithme
LOCAL_STATIC_LIBRARIES := stats_client map gui search storage indexer graphics platform anim geometry coding base expat freetype fribidi zlib bzip2 jansson tomcrypt protobuf
LOCAL_CFLAGS := -ffunction-sections -fdata-sections -Wno-psabi

ifneq ($(NDK_DEBUG),1)
  ifeq ($(PRODUCTION),1)
    OMIM_CONFIG := production
    LOCAL_CFLAGS += -fvisibility=hidden
    LOCAL_CPPFLAGS += -fvisibility-inlines-hidden
  endif
  LOCAL_CFLAGS += -O3
  LOCAL_LFLAGS += -O3
endif


TARGET_PLATFORM := android-5

ifeq ($(TARGET_ARCH_ABI), mips)
  TARGET_PLATFORM := android-9
else
  ifeq ($(TARGET_ARCH_ABI), x86)
    TARGET_PLATFORM := android-9
  endif
endif

LOCAL_HEADER_FILES := \
 	com/mapswithme/core/jni_helper.hpp \
	com/mapswithme/core/logging.hpp \
	com/mapswithme/core/render_context.hpp \
	com/mapswithme/maps/Framework.hpp \
	com/mapswithme/maps/MapStorage.hpp \
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
	com/mapswithme/maps/bookmarks/data/Bookmark.cpp \
	com/mapswithme/maps/bookmarks/data/BookmarkManager.cpp \
	com/mapswithme/maps/bookmarks/data/BookmarkCategory.cpp \
	com/mapswithme/maps/VideoTimer.cpp \
	com/mapswithme/maps/MWMActivity.cpp \
	com/mapswithme/maps/MWMApplication.cpp \
	com/mapswithme/maps/Lifecycle.cpp \
	com/mapswithme/maps/LocationState.cpp \
	com/mapswithme/maps/MapStorage.cpp \
	com/mapswithme/maps/DownloadResourcesActivity.cpp \
	com/mapswithme/maps/SearchActivity.cpp \
	com/mapswithme/maps/NativeEventTracker.cpp \
	com/mapswithme/maps/settings/StoragePathActivity.cpp \
	com/mapswithme/maps/settings/UnitLocale.cpp \
	com/mapswithme/maps/StoragePathManager.cpp \
	com/mapswithme/platform/Platform.cpp \
	com/mapswithme/platform/HttpThread.cpp \
	com/mapswithme/platform/Language.cpp \
	com/mapswithme/platform/PThreadImpl.cpp \
	nv_thread/nv_thread.cpp \
	nv_event/nv_event_queue.cpp \
	nv_event/nv_event.cpp \
	nv_time/nv_time.cpp

LOCAL_LDLIBS := -llog -lGLESv2

LOCAL_LDLIBS += -Wl,--gc-sections

include $(BUILD_SHARED_LIBRARY)
