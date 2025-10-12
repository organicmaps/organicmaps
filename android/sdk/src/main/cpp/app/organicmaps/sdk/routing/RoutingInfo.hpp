#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/routing/CarDirection.hpp"
#include "app/organicmaps/sdk/routing/LaneInfo.hpp"
#include "app/organicmaps/sdk/routing/PedestrianDirection.hpp"

#include "map/routing_manager.hpp"

jobject CreateRoutingInfo(JNIEnv * env, routing::FollowingInfo const & info, RoutingManager & rm)
{
  static jclass const klass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RoutingInfo");
  // Java signature : RoutingInfo(Distance distToTarget, Distance distToTurn, String currentStreet, String nextStreet,
  //                              String nextNextStreet, double completionPercent, CarDirection carTurnDirection,
  //                              CarDirection carNextTurnDirection, PedestrianDirection pedestrianDirection,
  //                              int exitNum, int totalTime, LaneInfo[] lanes, double speedLimitMps,
  //                              boolean speedLimitExceeded, boolean shouldPlayWarningSignal)
  static jmethodID const ctorRouteInfoID = jni::GetConstructorID(env, klass,
                                                                 "("
                                                                 "Lapp/organicmaps/sdk/util/Distance;"
                                                                 "Lapp/organicmaps/sdk/util/Distance;"
                                                                 "Ljava/lang/String;"
                                                                 "Ljava/lang/String;"
                                                                 "Ljava/lang/String;"
                                                                 "D"
                                                                 "Lapp/organicmaps/sdk/routing/CarDirection;"
                                                                 "Lapp/organicmaps/sdk/routing/CarDirection;"
                                                                 "Lapp/organicmaps/sdk/routing/PedestrianDirection;"
                                                                 "I"
                                                                 "I"
                                                                 "[Lapp/organicmaps/sdk/routing/LaneInfo;"
                                                                 "D"
                                                                 "Z"
                                                                 "Z"
                                                                 ")V");

  jobjectArray jLanes = CreateLanesInfo(env, info.m_lanes);

  auto const isSpeedCamLimitExceeded = rm.IsSpeedCamLimitExceeded();
  auto const shouldPlaySignal = rm.GetSpeedCamManager().ShouldPlayBeepSignal();
  jobject const result = env->NewObject(
      klass, ctorRouteInfoID, ToJavaDistance(env, info.m_distToTarget), ToJavaDistance(env, info.m_distToTurn),
      jni::ToJavaString(env, info.m_currentStreetName), jni::ToJavaString(env, info.m_nextStreetName),
      jni::ToJavaString(env, info.m_nextNextStreetName), info.m_completionPercent, ToJavaCarDirection(env, info.m_turn),
      ToJavaCarDirection(env, info.m_nextTurn), ToJavaPedestrianDirection(env, info.m_pedestrianTurn), info.m_exitNum,
      info.m_time, jLanes, info.m_speedLimitMps, static_cast<jboolean>(isSpeedCamLimitExceeded),
      static_cast<jboolean>(shouldPlaySignal));
  ASSERT(result, (jni::DescribeException()));
  return result;
}
