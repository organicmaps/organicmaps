NDK_TOOLCHAIN_VERSION := clang3.5
APP_PLATFORM := android-5
APP_STL := c++_static
#APP_CPPFLAGS += -std=c++11
# for gcc 4.6
#APP_CPPFLAGS += -D__GXX_EXPERIMENTAL_CXX0X__ -D_GLIBCXX_USE_C99_STDINT_TR1
# for gcc 4.8+
APP_CPPFLAGS += -Wno-deprecated-register

ifeq (x$(NDK_ABI_TO_BUILD), x)
  APP_ABI := armeabi-v7a-hard x86
else
  APP_ABI := $(NDK_ABI_TO_BUILD)
endif

LOCAL_PATH := $(call my-dir)
APP_CFLAGS += -I$(LOCAL_PATH)/../../3party/boost \
              -I$(LOCAL_PATH)/../../3party/protobuf/src

APP_GNUSTL_FORCE_CPP_FEATURES := exceptions rtti

ifeq ($(NDK_DEBUG),1)
  APP_OPTIM := debug
  APP_CFLAGS += -DDEBUG -D_DEBUG
else
  APP_OPTIM := release
  APP_CFLAGS += -DRELEASE -D_RELEASE
ifeq ($(PRODUCTION),1)
  APP_CFLAGS += -DOMIM_PRODUCTION
endif
endif
