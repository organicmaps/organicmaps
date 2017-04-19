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

prebuild_static_libs := map tracking routing traffic routing_common drape_frontend search storage
prebuild_static_libs += indexer drape platform editor partners_api local_ads geometry coding base
prebuild_static_libs += opening_hours pugixml oauthcpp expat freetype minizip jansson
prebuild_static_libs += protobuf osrm stats_client succinct stb_image sdf_image icu

$(foreach item,$(prebuild_static_libs),$(eval $(call add_prebuild_static_lib,$(item))))

endif

########################### Main MapsWithMe module ############################

include $(CLEAR_VARS)

LOCAL_CPP_FEATURES += exceptions rtti

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../

LOCAL_MODULE := mapswithme
LOCAL_STATIC_LIBRARIES := $(prebuild_static_libs)

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
	com/mapswithme/core/ScopedEnv.hpp \
	com/mapswithme/core/ScopedLocalRef.hpp \
	com/mapswithme/maps/Framework.hpp \
	com/mapswithme/opengl/android_gl_utils.hpp \
	com/mapswithme/opengl/androidoglcontext.hpp \
	com/mapswithme/opengl/androidoglcontextfactory.hpp \
	com/mapswithme/platform/Platform.hpp \

LOCAL_SRC_FILES := \
	com/mapswithme/core/jni_helper.cpp \
	com/mapswithme/core/logging.cpp \
	com/mapswithme/maps/bookmarks/data/Bookmark.cpp \
	com/mapswithme/maps/bookmarks/data/BookmarkManager.cpp \
	com/mapswithme/maps/bookmarks/data/BookmarkCategory.cpp \
	com/mapswithme/maps/DisplayedCategories.cpp \
	com/mapswithme/maps/DownloadResourcesActivity.cpp \
	com/mapswithme/maps/editor/OpeningHours.cpp \
	com/mapswithme/maps/editor/Editor.cpp \
	com/mapswithme/maps/editor/OsmOAuth.cpp \
	com/mapswithme/maps/Framework.cpp \
	com/mapswithme/maps/LocationState.cpp \
	com/mapswithme/maps/LocationHelper.cpp \
	com/mapswithme/maps/MapFragment.cpp \
	com/mapswithme/maps/MapManager.cpp \
	com/mapswithme/maps/MwmApplication.cpp \
	com/mapswithme/maps/PrivateVariables.cpp \
	com/mapswithme/maps/SearchEngine.cpp \
	com/mapswithme/maps/SearchRecents.cpp \
	com/mapswithme/maps/settings/UnitLocale.cpp \
	com/mapswithme/maps/sound/tts.cpp \
	com/mapswithme/maps/Sponsored.cpp \
	com/mapswithme/maps/uber/Uber.cpp \
	com/mapswithme/maps/TrackRecorder.cpp \
	com/mapswithme/maps/TrafficState.cpp \
	com/mapswithme/maps/UserMarkHelper.cpp \
	com/mapswithme/opengl/android_gl_utils.cpp \
	com/mapswithme/opengl/androidoglcontext.cpp \
	com/mapswithme/opengl/androidoglcontextfactory.cpp \
	com/mapswithme/platform/HttpThread.cpp \
	com/mapswithme/platform/SocketImpl.cpp \
	com/mapswithme/platform/Language.cpp \
	com/mapswithme/platform/MarketingService.cpp \
	com/mapswithme/platform/Platform.cpp \
	com/mapswithme/platform/PThreadImpl.cpp \
	com/mapswithme/util/Config.cpp \
	com/mapswithme/util/HttpClient.cpp \
	com/mapswithme/util/StringUtils.cpp \
	com/mapswithme/util/statistics/PushwooshHelper.cpp \
	com/mapswithme/util/LoggerFactory.cpp \
	com/mapswithme/util/NetworkPolicy.cpp \


LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2 -latomic -lz

LOCAL_LDLIBS += -Wl,--gc-sections

include $(BUILD_SHARED_LIBRARY)
