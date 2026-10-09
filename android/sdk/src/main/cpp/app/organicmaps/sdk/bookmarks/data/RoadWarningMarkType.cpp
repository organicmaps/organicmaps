#include "RoadWarningMarkType.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/routing_mark.hpp"

jobject CreateRoadWarningMarkType(JNIEnv * env, RoadWarningMarkType const roadWarningMarkType)
{
  static jclass clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/RoadWarningMarkType");
  static jmethodID getter =
      jni::GetStaticMethodID(env, clazz, "fromInt", "(I)Lapp/organicmaps/sdk/bookmarks/data/RoadWarningMarkType;");
  return env->CallStaticObjectMethod(clazz, getter, static_cast<jint>(roadWarningMarkType));
}
