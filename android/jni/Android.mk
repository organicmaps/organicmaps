LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#LOCAL_CPP_FEATURES += exceptions rtti

LOCAL_MODULE    := mapswithme

#LOCAL_CFLAGS := -DANDROID_NDK \
#                -DDISABLE_IMPORTGL

LOCAL_CFLAGS := -ffunction-sections -fdata-sections

TARGET_PLATFORM := android-5

LOCAL_HEADER_FILES := \
 	com/mapswithme/core/jni_helper.hpp \
	com/mapswithme/core/jni_string.hpp \
	com/mapswithme/core/logging.hpp \
	com/mapswithme/core/render_context.hpp \
	com/mapswithme/maps/Framework.hpp \
	com/mapswithme/platform/Platform.hpp \
  com/mapswithme/platform/http_thread_android.hpp \

LOCAL_SRC_FILES := \
	com/mapswithme/core/concurrent_runner.cpp \
	com/mapswithme/core/jni_helper.cpp \
	com/mapswithme/core/jni_string.cpp \
	com/mapswithme/core/logging.cpp \
	com/mapswithme/core/render_context.cpp \
	com/mapswithme/location/LocationService.cpp \
	com/mapswithme/maps/DownloadUI.cpp \
	com/mapswithme/maps/Framework.cpp \
	com/mapswithme/maps/VideoTimer.cpp \
	com/mapswithme/maps/GesturesProcessor.cpp \
	com/mapswithme/maps/MainGLView.cpp \
	com/mapswithme/maps/MWMActivity.cpp \
	com/mapswithme/platform/Platform.cpp \
	com/mapswithme/platform/http_thread_android.cpp \
	
LOCAL_LDLIBS := -llog -lGLESv1_CM \
  	-lwords -lmap -lversion -lsearch -lstorage -lindexer -lyg -lplatform \
  	-lgeometry -lcoding -lbase -lexpat -lfreetype -lfribidi -lzlib -lbzip2 \
	-ljansson -ltomcrypt -lprotobuf ./obj/local/armeabi/libstdc++.a

LOCAL_LDLIBS += -L../../omim-android-debug/out/debug
#LOCAL_LDLIBS += -L../../omim-android-release/out/release

LOCAL_LDLIBS += -Wl,--gc-sections

include $(BUILD_SHARED_LIBRARY)
