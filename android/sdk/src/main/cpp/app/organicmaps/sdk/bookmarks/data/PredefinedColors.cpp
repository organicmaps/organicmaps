#include "PredefinedColors.hpp"

#include "kml/types.hpp"

#include <array>

extern "C"
{
JNIEXPORT jintArray JNICALL
Java_app_organicmaps_sdk_bookmarks_data_PredefinedColors_nativeGetPredefinedColors(JNIEnv * env, jclass)
{
  using kml::kOrderedPredefinedColors;
  std::array<jint, kOrderedPredefinedColors.size()> colors;
  for (size_t i = 0; i < kOrderedPredefinedColors.size(); ++i)
    colors[i] = static_cast<jint>(kml::ColorFromPredefinedColor(kOrderedPredefinedColors[i]).GetARGB());

  jintArray jColors = env->NewIntArray(colors.size());
  env->SetIntArrayRegion(jColors, 0, static_cast<jsize>(colors.size()), colors.data());
  return jColors;
}
}

namespace
{
JNINativeMethod const predefinedColorsMethods[] = {
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

  return env->RegisterNatives(clazz, predefinedColorsMethods, std::size(predefinedColorsMethods));
}
}  // namespace predefined_colors
