#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "geometry/point_with_altitude.hpp"

#include <vector>

jobjectArray CreateJunctionInfoArray(JNIEnv * env, std::vector<geometry::PointWithAltitude> const & junctionPoints)
{
  static jclass const junctionClazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/JunctionInfo");
  // Java signature : JunctionInfo(double lat, double lon)
  static jmethodID const junctionConstructor = jni::GetConstructorID(env, junctionClazz, "(DD)V");

  return jni::ToJavaArray(env, junctionClazz, junctionPoints,
                          [](JNIEnv * env, geometry::PointWithAltitude const & pointWithAltitude)
                          {
                            auto & point = pointWithAltitude.GetPoint();
                            return env->NewObject(junctionClazz, junctionConstructor, mercator::YToLat(point.y),
                                                  mercator::XToLon(point.x));
                          });
}
