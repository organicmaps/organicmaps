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

# Avoid clean errors due to missing external static libs
ifneq ($(MAKECMDGOALS),clean)

define add_prebuild_static_lib
  include $(CLEAR_VARS)
  LOCAL_MODULE := $1
  LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/lib$1.a
  include $(PREBUILT_STATIC_LIBRARY)
endef

prebuild_static_libs := osrm protobuf tomcrypt jansson minizip fribidi freetype expat base normalize coding geometry anim platform graphics indexer storage search routing gui render map stats_client succinct opening_hours

$(foreach item,$(prebuild_static_libs),$(eval $(call add_prebuild_static_lib,$(item))))

endif

########################### Main MapsWithMe module ############################

include $(CLEAR_VARS)

LOCAL_CPP_FEATURES += exceptions rtti

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../

LOCAL_MODULE := mapswithme
LOCAL_STATIC_LIBRARIES := map render gui routing search storage indexer graphics platform anim geometry coding normalize base expat freetype fribidi minizip jansson tomcrypt protobuf osrm stats_client succinct opening_hours
LOCAL_CFLAGS := -ffunction-sections -fdata-sections -Wno-extern-c-compat

ifneq ($(NDK_DEBUG),1)
  ifeq ($(PRODUCTION),1)
    OMIM_CONFIG := production
    LOCAL_CFLAGS += -fvisibility=hidden
    LOCAL_CPPFLAGS += -fvisibility-inlines-hidden
  endif
  LOCAL_CFLAGS += -O3
  LOCAL_LFLAGS += -O3
endif


TARGET_PLATFORM := android-15

LOCAL_HEADER_FILES := \
	../../private.h \
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
	com/mapswithme/country/country_helper.cpp \
	com/mapswithme/country/CountryTree.cpp \
	com/mapswithme/country/ActiveCountryTree.cpp \
	com/mapswithme/maps/Framework.cpp \
	com/mapswithme/maps/bookmarks/data/Bookmark.cpp \
	com/mapswithme/maps/bookmarks/data/BookmarkManager.cpp \
	com/mapswithme/maps/bookmarks/data/BookmarkCategory.cpp \
	com/mapswithme/maps/sound/tts.cpp \
	com/mapswithme/maps/VideoTimer.cpp \
	com/mapswithme/maps/MapFragment.cpp \
	com/mapswithme/maps/MwmApplication.cpp \
	com/mapswithme/maps/Lifecycle.cpp \
	com/mapswithme/maps/LocationState.cpp \
	com/mapswithme/maps/MapStorage.cpp \
	com/mapswithme/maps/DownloadResourcesActivity.cpp \
	com/mapswithme/maps/PrivateVariables.cpp \
	com/mapswithme/maps/SearchFragment.cpp \
	com/mapswithme/maps/settings/UnitLocale.cpp \
	com/mapswithme/platform/Platform.cpp \
	com/mapswithme/platform/HttpThread.cpp \
	com/mapswithme/platform/Language.cpp \
	com/mapswithme/platform/PThreadImpl.cpp \
	com/mapswithme/utils/StringUtils.cpp \
	nv_thread/nv_thread.cpp \
	nv_event/nv_event_queue.cpp \
	nv_event/nv_event.cpp \
	nv_time/nv_time.cpp \

LOCAL_LDLIBS := -llog -lGLESv2 -latomic -lz

LOCAL_LDLIBS += -Wl,--gc-sections

include $(BUILD_SHARED_LIBRARY)
