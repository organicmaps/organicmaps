cmake_minimum_required(VERSION 3.22.1)

project(organicmaps C CXX)

set(SRC
  # JNI headers
  ../../../../../private.h
  app/organicmaps/sdk/core/jni_helper.hpp
  app/organicmaps/sdk/core/jni_java_methods.hpp
  app/organicmaps/sdk/core/logging.hpp
  app/organicmaps/sdk/core/ScopedEnv.hpp
  app/organicmaps/sdk/core/ScopedLocalRef.hpp
  app/organicmaps/sdk/Framework.hpp
  app/organicmaps/sdk/opengl/android_gl_utils.hpp
  app/organicmaps/sdk/opengl/androidoglcontext.hpp
  app/organicmaps/sdk/opengl/androidoglcontextfactory.hpp
  app/organicmaps/sdk/opengl/gl3stub.h
  app/organicmaps/sdk/platform/GuiThread.hpp
  app/organicmaps/sdk/platform/AndroidPlatform.hpp
  app/organicmaps/sdk/util/Distance.hpp
  app/organicmaps/sdk/util/FeatureIdBuilder.hpp
  app/organicmaps/sdk/vulkan/android_vulkan_context_factory.hpp

  # JNI sources
  app/organicmaps/sdk/search/DisplayedCategories.cpp
  app/organicmaps/sdk/search/SearchEngine.cpp
  app/organicmaps/sdk/search/SearchRecents.cpp
  app/organicmaps/sdk/routing/JunctionInfo.hpp
  app/organicmaps/sdk/routing/RouteMarkData.hpp
  app/organicmaps/sdk/routing/RouteMarkType.hpp
  app/organicmaps/sdk/routing/RoutePointInfo.hpp
  app/organicmaps/sdk/routing/RouteRecommendationType.hpp
  app/organicmaps/sdk/routing/RoutingInfo.hpp
  app/organicmaps/sdk/routing/RoutingOptions.cpp
  app/organicmaps/sdk/routing/SingleLaneInfo.hpp
  app/organicmaps/sdk/routing/TransitRouteInfo.hpp
  app/organicmaps/sdk/routing/TransitStepInfo.hpp
  app/organicmaps/sdk/ChoosePositionMode.cpp
  app/organicmaps/sdk/MapStyle.cpp
  app/organicmaps/sdk/OrganicMaps.cpp
  app/organicmaps/sdk/Router.cpp
  app/organicmaps/sdk/core/jni_helper.cpp
  app/organicmaps/sdk/core/jni_java_methods.cpp
  app/organicmaps/sdk/core/logging.cpp
  app/organicmaps/sdk/bookmarks/data/BookmarkManager.cpp
  app/organicmaps/sdk/bookmarks/data/Icon.cpp
  app/organicmaps/sdk/bookmarks/data/Icon.hpp
  app/organicmaps/sdk/bookmarks/data/PredefinedColors.cpp
  app/organicmaps/sdk/bookmarks/data/PredefinedColors.hpp
  app/organicmaps/sdk/DownloadResourcesLegacyActivity.cpp
  app/organicmaps/sdk/editor/Editor.cpp
  app/organicmaps/sdk/editor/OpeningHours.cpp
  app/organicmaps/sdk/editor/OsmOAuth.cpp
  app/organicmaps/sdk/Framework.cpp
  app/organicmaps/sdk/isolines/IsolinesManager.cpp
  app/organicmaps/sdk/LocationState.cpp
  app/organicmaps/sdk/Map.cpp
  app/organicmaps/sdk/MapManager.cpp
  app/organicmaps/sdk/settings/UnitLocale.cpp
  app/organicmaps/sdk/settings/MapLanguageCode.cpp
  app/organicmaps/sdk/sound/tts.cpp
  app/organicmaps/sdk/subway/SubwayManager.cpp
  app/organicmaps/sdk/TrackRecorder.cpp
  app/organicmaps/sdk/TrafficState.cpp
  app/organicmaps/sdk/UserMarkHelper.cpp
  app/organicmaps/sdk/opengl/android_gl_utils.cpp
  app/organicmaps/sdk/opengl/androidoglcontext.cpp
  app/organicmaps/sdk/opengl/androidoglcontextfactory.cpp
  app/organicmaps/sdk/opengl/gl3stub.cpp
  app/organicmaps/sdk/platform/GuiThread.cpp
  app/organicmaps/sdk/platform/HttpThread.cpp
  app/organicmaps/sdk/platform/Language.cpp
  app/organicmaps/sdk/platform/Localization.cpp
  app/organicmaps/sdk/platform/AndroidPlatform.cpp
  app/organicmaps/sdk/platform/PThreadImpl.cpp
  app/organicmaps/sdk/platform/SecureStorage.cpp
  app/organicmaps/sdk/platform/SocketImpl.cpp
  app/organicmaps/sdk/sync/SyncManager.cpp
  app/organicmaps/sdk/util/Config.cpp
  app/organicmaps/sdk/util/FileUtils.cpp
  app/organicmaps/sdk/util/GeoUtils.cpp
  app/organicmaps/sdk/util/HttpClient.cpp
  app/organicmaps/sdk/util/Language.cpp
  app/organicmaps/sdk/util/LogsManager.cpp
  app/organicmaps/sdk/util/NetworkPolicy.cpp
  app/organicmaps/sdk/util/StringUtils.cpp
  app/organicmaps/sdk/util/UiThread.cpp
  app/organicmaps/sdk/vulkan/android_vulkan_context_factory.cpp
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
  # expat
  # freetype
  # minizip
  # cppjansson
  # protobuf
  # succinct
  # stb_image
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
