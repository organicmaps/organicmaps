#include <jni.h>

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "kml/types.hpp"

extern "C"
{
JNIEXPORT jobjectArray JNICALL Java_app_organicmaps_sdk_bookmarks_data_Icon_nativeGetBookmarkIconNames(JNIEnv * env,
                                                                                                       jclass)
{
  std::vector<std::string> icons;
  for (uint16_t i = 0; i < static_cast<uint16_t>(kml::BookmarkIcon::Count); ++i)
    icons.emplace_back(kml::DebugPrint(static_cast<kml::BookmarkIcon>(i)));
  return jni::ToJavaStringArray(env, icons);
}
}

namespace
{
JNINativeMethod const iconMethods[] = {
    {"nativeGetBookmarkIconNames", "()[Ljava/lang/String;",
     reinterpret_cast<void *>(&Java_app_organicmaps_sdk_bookmarks_data_Icon_nativeGetBookmarkIconNames)},
};
}

namespace icon
{
jint registerNativeMethods(JNIEnv * env)
{
  jclass clazz = env->FindClass("app/organicmaps/sdk/bookmarks/data/Icon");
  if (clazz == nullptr)
    return JNI_ERR;

  return env->RegisterNatives(clazz, iconMethods, std::size(iconMethods));
}
}  // namespace icon
