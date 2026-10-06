#include "OpeningMode.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/place_page_info.hpp"

jobject CreateOpeningMode(JNIEnv * env, place_page::OpeningMode openingMode)
{
  static jclass clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/OpeningMode");
  static jmethodID getter =
      jni::GetStaticMethodID(env, clazz, "fromInt", "(I)Lapp/organicmaps/sdk/bookmarks/data/OpeningMode;");
  return env->CallStaticObjectMethod(clazz, getter, static_cast<jint>(openingMode));
}
