APP_STL := gnustl_static
APP_CFLAGS += -I../3party/boost
APP_CPPFLAGS += -fexceptions -frtti

# comment this to enable release build
APP_OPTIM := debug
APP_CFLAGS += -DDEBUG -D_DEBUG