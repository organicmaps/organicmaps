#pragma once

#include "app/organicmaps/core/jni_helper.hpp"

#include "platform/speed_formatted.hpp"

inline jobject ToJavaSpeedFormatted(JNIEnv * env, platform::SpeedFormatted const & speedFormatted)
{
  static jclass const speedFormattedClass = jni::GetGlobalClassRef(env, "app/organicmaps/util/SpeedFormatted");

  static jmethodID const speedFormattedConstructor = jni::GetConstructorID(env, speedFormattedClass, "(DLjava/lang/String;B)V");

  jobject distanceObject = env->NewObject(
      speedFormattedClass, speedFormattedConstructor,
      speedFormatted.GetSpeed(), jni::ToJavaString(env, speedFormatted.GetSpeedString()), static_cast<uint8_t>(speedFormatted.GetUnits()));

  return distanceObject;
}
