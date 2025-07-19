#include "PredefinedColors.hpp"

#include "kml/types.hpp"

#include <array>

extern "C"
{
JNIEXPORT jintArray JNICALL
Java_app_organicmaps_sdk_bookmarks_data_PredefinedColors_nativeGetPredefinedColors(JNIEnv * env, jclass)
{
  std::array<jint, static_cast<size_t>(kml::PredefinedColor::Count)> colors{};
  for (size_t i = 0; i < static_cast<size_t>(kml::PredefinedColor::Count); ++i)
    colors[i] = static_cast<jint>(kml::ColorFromPredefinedColor(static_cast<kml::PredefinedColor>(i)).GetARGB());

  jintArray jColors = env->NewIntArray(colors.size());
  env->SetIntArrayRegion(jColors, 0, static_cast<jsize>(colors.size()), colors.data());
  return jColors;
}
}

namespace
{
JNINativeMethod const methods[] = {
    {"nativeGetPredefinedColors", "()[I",
     reinterpret_cast<void *>(&Java_app_organicmaps_sdk_bookmarks_data_PredefinedColors_nativeGetPredefinedColors)},
};
}

namespace predefined_colors
{
jint registerNativeMethods(JNIEnv * env)
{
  jclass clazz = env->FindClass("app/organicmaps/sdk/bookmarks/data/PredefinedColors");
  if (clazz == nullptr)
    return JNI_ERR;

  return env->RegisterNatives(clazz, methods, std::size(methods));
}
}  // namespace predefined_colors
