#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include <vector>

jobjectArray CreateJunctionInfoArray(JNIEnv * env, std::vector<geometry::PointWithAltitude> const & junctionPoints)
{
  static jclass const junctionClazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/JunctionInfo");
  // Java signature : JunctionInfo(double lat, double lon)
  static jmethodID const junctionConstructor = jni::GetConstructorID(env, junctionClazz, "(DD)V");

  return jni::ToJavaArray(env, junctionClazz, junctionPoints,
                          [](JNIEnv * env, geometry::PointWithAltitude const & pointWithAltitude)
  {
    auto const & ll = pointWithAltitude.ToLatLon();
    return env->NewObject(junctionClazz, junctionConstructor, ll.m_lat, ll.m_lon);
  });
}
