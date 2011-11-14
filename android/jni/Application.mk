APP_PLATFORM := android-5
APP_ABI := armeabi
APP_STL := gnustl_static
APP_CFLAGS += -I../3party/boost

# comment/uncomment this to enable release build
APP_OPTIM := debug
APP_CFLAGS += -DDEBUG -D_DEBUG
#APP_OPTIM := release
#APP_CFLAGS += -DRELEASE -D_RELEASE
