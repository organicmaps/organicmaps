LOCAL_PATH := $(call my-dir)

OMIM_CONFIG := release
ifeq ($(NDK_DEBUG),1)
  OMIM_CONFIG := debug
else
  ifeq ($(PRODUCTION),1)
    OMIM_CONFIG := production
    LOCAL_CFLAGS += -fvisibility=hidden
    LOCAL_CPPFLAGS += -fvisibility-inlines-hidden
  endif
  LOCAL_CFLAGS += -O3
endif

$(info "***** Building $(OMIM_CONFIG) configuration for $(TARGET_ARCH_ABI) *****")

# Build static libraries

BUILD_RESULT := $(shell (bash $(LOCAL_PATH)/../../../tools/autobuild/android.sh $(OMIM_CONFIG) $(TARGET_ARCH_ABI) $(APP_PLATFORM)))

####################################################################################
# List all static libraries which are built using our own scripts in tools/android #
####################################################################################

MY_PREBUILT_LIBS_PATH := ../../../../omim-android-$(OMIM_CONFIG)-$(TARGET_ARCH_ABI)/out/$(OMIM_CONFIG)

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

########################### Main YoPMe module ############################

include $(CLEAR_VARS)

LOCAL_MODULE := yopme
LOCAL_STATIC_LIBRARIES := map gui search storage indexer graphics platform anim geometry coding base expat freetype fribidi zlib bzip2 jansson tomcrypt protobuf
LOCAL_CFLAGS := -ffunction-sections -fdata-sections -Wno-psabi

TARGET_PLATFORM := android-15

# Add your headers below
LOCAL_HEADER_FILES := Stubs.hpp \
                      Framework.hpp \
                      ../../jni/com/mapswithme/core/jni_helper.hpp \
                      ../../jni/com/mapswithme/core/logging.hpp \
                      ../../jni/com/mapswithme/platform/Platform.hpp \

# Add your sources below
LOCAL_SRC_FILES := Stubs.cpp \
                   Framework.cpp \
                   MapRenderer.cpp \
                   BackscreenActivity.cpp \
                   ../../jni/com/mapswithme/core/logging.cpp \
                   ../../jni/com/mapswithme/core/jni_helper.cpp \
                   ../../jni/com/mapswithme/platform/Platform.cpp \
                   ../../jni/com/mapswithme/platform/HttpThread.cpp \
                   ../../jni/com/mapswithme/platform/Language.cpp \
                   ../../jni/com/mapswithme/platform/PThreadImpl.cpp \

LOCAL_LDLIBS := -llog -lGLESv2

LOCAL_LDLIBS += -Wl,--gc-sections

include $(BUILD_SHARED_LIBRARY)
