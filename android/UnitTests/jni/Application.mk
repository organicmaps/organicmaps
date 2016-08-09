NDK_TOOLCHAIN_VERSION := clang
APP_ABI := armeabi-v7a

APP_PLATFORM := android-15
APP_STL := c++_static

# libc++-specific issues: -std=c++11" is turned on by default.

APP_CPPFLAGS += -Wno-deprecated-register

#@todo(vbykoianko) Build tests for android x86 platform
#ifeq (x$(NDK_ABI_TO_BUILD), x)
#  APP_ABI := armeabi-v7a x86
#else
#  APP_ABI := $(NDK_ABI_TO_BUILD)
#endif

APP_CPPFLAGS += -fexceptions
LOCAL_CPPFLAGS += -fexceptions

APP_CPPFLAGS += -frtti
LOCAL_CPPFLAGS += -frtti

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
