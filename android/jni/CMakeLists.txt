cmake_minimum_required(VERSION 3.22.1)

project(organicmaps C CXX)

set(SRC
  # JNI headers
  ../../private.h
  com/mapswithme/core/jni_helper.hpp
  com/mapswithme/core/logging.hpp
  com/mapswithme/core/ScopedEnv.hpp
  com/mapswithme/core/ScopedLocalRef.hpp
  com/mapswithme/maps/Framework.hpp
  com/mapswithme/opengl/android_gl_utils.hpp
  com/mapswithme/opengl/androidoglcontext.hpp
  com/mapswithme/opengl/androidoglcontextfactory.hpp
  com/mapswithme/opengl/gl3stub.h
  com/mapswithme/platform/GuiThread.hpp
  com/mapswithme/platform/Platform.hpp
  com/mapswithme/util/FeatureIdBuilder.hpp
  com/mapswithme/vulkan/android_vulkan_context_factory.hpp

  # JNI sources
  com/mapswithme/core/jni_helper.cpp
  com/mapswithme/core/logging.cpp
  com/mapswithme/maps/bookmarks/data/BookmarkManager.cpp
  com/mapswithme/maps/DisplayedCategories.cpp
  com/mapswithme/maps/DownloadResourcesLegacyActivity.cpp
  com/mapswithme/maps/editor/Editor.cpp
  com/mapswithme/maps/editor/OpeningHours.cpp
  com/mapswithme/maps/editor/OsmOAuth.cpp
  com/mapswithme/maps/Framework.cpp
  com/mapswithme/maps/isolines/IsolinesManager.cpp
  com/mapswithme/maps/LocationHelper.cpp
  com/mapswithme/maps/LocationState.cpp
  com/mapswithme/maps/MapFragment.cpp
  com/mapswithme/maps/MapManager.cpp
  com/mapswithme/maps/MwmApplication.cpp
  com/mapswithme/maps/routing/RoutingOptions.cpp
  com/mapswithme/maps/SearchEngine.cpp
  com/mapswithme/maps/SearchRecents.cpp
  com/mapswithme/maps/settings/UnitLocale.cpp
  com/mapswithme/maps/sound/tts.cpp
  com/mapswithme/maps/subway/SubwayManager.cpp
  com/mapswithme/maps/TrackRecorder.cpp
  com/mapswithme/maps/TrafficState.cpp
  com/mapswithme/maps/UserMarkHelper.cpp
  com/mapswithme/opengl/android_gl_utils.cpp
  com/mapswithme/opengl/androidoglcontext.cpp
  com/mapswithme/opengl/androidoglcontextfactory.cpp
  com/mapswithme/opengl/gl3stub.c
  com/mapswithme/platform/GuiThread.cpp
  com/mapswithme/platform/HttpThread.cpp
  com/mapswithme/platform/Language.cpp
  com/mapswithme/platform/Localization.cpp
  com/mapswithme/platform/Platform.cpp
  com/mapswithme/platform/PThreadImpl.cpp
  com/mapswithme/platform/SecureStorage.cpp
  com/mapswithme/platform/SocketImpl.cpp
  com/mapswithme/util/Config.cpp
  com/mapswithme/util/GeoUtils.cpp
  com/mapswithme/util/HttpBackgroundUploader.cpp
  com/mapswithme/util/HttpClient.cpp
  com/mapswithme/util/HttpUploader.cpp
  com/mapswithme/util/HttpUploaderUtils.cpp
  com/mapswithme/util/Language.cpp
  com/mapswithme/util/LogsManager.cpp
  com/mapswithme/util/NetworkPolicy.cpp
  com/mapswithme/util/StringUtils.cpp
  com/mapswithme/vulkan/android_vulkan_context_factory.cpp
)

omim_add_library(${PROJECT_NAME} SHARED ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE .)

target_link_libraries(${PROJECT_NAME}
  # Organic Maps libs
  map
  # ge0
  # tracking
  # routing
  # traffic
  # routing_common
  # drape_frontend
  # shaders
  # search
  # storage
  # descriptions
  # drape
  # kml
  # editor
  # transit
  # indexer
  # platform
  # mwm_diff
  # bsdiff
  # geometry
  # coding
  # base
  # opening_hours
  # pugixml
  # oauthcpp
  # expat
  # freetype
  # minizip
  # jansson
  # protobuf
  # succinct
  # stb_image
  # sdf_image
  # icu
  # agg
  # vulkan_wrapper

  # Android libs
  log
  android
  EGL
  GLESv2
  atomic
  z
)

#target_link_options(${PROJECT_NAME} PRIVATE "-fuse-ld=gold")
