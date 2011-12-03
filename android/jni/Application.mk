APP_PLATFORM := android-5
APP_ABI := armeabi
APP_STL := gnustl_static
APP_CFLAGS += -I../3party/boost

ifeq ($(NDK_DEBUG),1)
  APP_OPTIM := debug
  APP_CFLAGS += -DDEBUG -D_DEBUG
else
  APP_OPTIM := release
  APP_CFLAGS += -DRELEASE -D_RELEASE
endif