APP_PLATFORM := android-5
APP_ABI := armeabi armeabi-v7a mips x86
APP_STL := gnustl_static
LOCAL_PATH := $(call my-dir)
APP_CFLAGS += -I$(LOCAL_PATH)/../../3party/boost

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
