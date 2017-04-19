
OMIM_CONFIG := debug
ifeq ($(NDK_DEBUG),1)
  OMIM_CONFIG := debug
else
  ifeq ($(PRODUCTION),1)
    OMIM_CONFIG := production
  endif
endif

# Avoid clean errors due to missing external static libs
ifneq ($(MAKECMDGOALS),clean)

ROOT_PATH_FROM_JNI = $(ROOT_PATH)/..

MY_PREBUILT_LIBS_PATH := $(ROOT_PATH_FROM_JNI)/../omim-android-$(OMIM_CONFIG)-$(TARGET_ARCH_ABI)/out/$(OMIM_CONFIG)
$(info $(MY_PREBUILT_LIBS_PATH))

define add_prebuild_static_lib
  include $(CLEAR_VARS)
  LOCAL_MODULE := $1
  LOCAL_SRC_FILES := $(MY_PREBUILT_LIBS_PATH)/lib$1.a
  include $(PREBUILT_STATIC_LIBRARY)
endef

prebuild_static_libs := minizip osrm protobuf jansson freetype expat base coding geometry platform indexer storage search routing map opening_hours stats_client succinct

$(foreach item,$(prebuild_static_libs),$(eval $(call add_prebuild_static_lib,$(item))))

endif

include $(CLEAR_VARS)

LOCAL_CPP_FEATURES += exceptions rtti

LOCAL_C_INCLUDES := $(ROOT_PATH_FROM_JNI)/
LOCAL_C_INCLUDES += $(ROOT_PATH)/
LOCAL_C_INCLUDES += $(ROOT_PATH)/3party/boost/

ifneq ($(NDK_DEBUG),1)
  ifeq ($(PRODUCTION),1)
    OMIM_CONFIG := production
    LOCAL_CPPFLAGS += -fvisibility-inlines-hidden
  endif
  LOCAL_CFLAGS += -O3
  LOCAL_LFLAGS += -O3
endif
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Wno-extern-c-compat

TARGET_PLATFORM := android-9

$(info $(LOCAL_PATH))

MY_CPP_PATH = $(LOCAL_PATH)/..
MY_LOCAL_SRC_FILES := $(wildcard $(MY_CPP_PATH)/*.cpp)
LOCAL_SRC_FILES += $(subst jni/, , $(MY_LOCAL_SRC_FILES))

LOCAL_SRC_FILES += $(ROOT_PATH_FROM_JNI)/testing/testingmain.cpp
LOCAL_SRC_FILES += $(ROOT_PATH_FROM_JNI)/android/UnitTests/jni/mock.cpp
LOCAL_SRC_FILES += ./test.cpp

LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv1_CM
LOCAL_LDLIBS += -lGLESv2 -latomic -lz
LOCAL_LDLIBS += -Wl,--gc-sections
