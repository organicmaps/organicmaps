#pragma once

#include "app/organicmaps/sdk/routing/RouteMarkType.hpp"

#include "map/routing_mark.hpp"

RouteMarkType GetRouteMarkType(JNIEnv * env, jobject markType)
{
  static jmethodID const ordinal = jni::GetMethodID(env, markType, "ordinal", "()I");

  return static_cast<RouteMarkType>(env->CallIntMethod(markType, ordinal));
}
