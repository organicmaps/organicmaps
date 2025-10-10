#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/routing/CarDirection.hpp"
#include "app/organicmaps/sdk/routing/LaneInfo.hpp"
#include "app/organicmaps/sdk/routing/PedestrianDirection.hpp"
#include "app/organicmaps/sdk/routing/roadshield/RoadShieldInfo.hpp"

#include "map/routing_manager.hpp"

jobject CreateRoutingInfo(JNIEnv * env, routing::FollowingInfo const & info, RoutingManager & rm)
{
  static jclass const klass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RoutingInfo");
  // clang-format off
  static jmethodID const ctorRouteInfoID = jni::GetConstructorID(env, klass,
    "("
    "Lapp/organicmaps/sdk/util/Distance;"                      // distToTarget
    "Lapp/organicmaps/sdk/util/Distance;"                      // distToTurn
    "Ljava/lang/String;"                                       // currentStreet
    "Ljava/lang/String;"                                       // nextStreet
    "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldInfo;"  // nextStreetRoadShields
    "Ljava/lang/String;"                                       // nextNextStreet
    "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldInfo;"  // nextNextStreetRoadShields
    "D"                                                        // completionPercent
    "Lapp/organicmaps/sdk/routing/CarDirection;"               // carTurnDirection
    "Lapp/organicmaps/sdk/routing/CarDirection;"               // carNextTurnDirection
    "Lapp/organicmaps/sdk/routing/PedestrianDirection;"        // pedestrianDirection
    "I"                                                        // exitNum
    "I"                                                        // totalTime
    "[Lapp/organicmaps/sdk/routing/LaneInfo;"                  // lanes
    "D"                                                        // speedLimitMps
    "Z"                                                        // speedLimitExceeded
    "Z"                                                        // shouldPlayWarningSignal
    ")V"
  );
  // clang-format on

  // clang-format off
  jobject const result = env->NewObject(klass, ctorRouteInfoID,
    ToJavaDistance(env, info.m_distToTarget),
    ToJavaDistance(env, info.m_distToTurn),
    jni::ToJavaString(env, info.m_currentStreetName),
    jni::ToJavaString(env, info.m_nextStreetName),
    ToJavaRoadShieldInfo(env, info.m_nextStreetShields),
    jni::ToJavaString(env, info.m_nextNextStreetName),
    ToJavaRoadShieldInfo(env, info.m_nextNextStreetShields),
    info.m_completionPercent,
    ToJavaCarDirection(env, info.m_turn),
    ToJavaCarDirection(env, info.m_nextTurn),
    ToJavaPedestrianDirection(env, info.m_pedestrianTurn),
    info.m_exitNum,
    info.m_time,
    CreateLanesInfo(env, info.m_lanes),
    info.m_speedLimitMps,
    static_cast<jboolean>(rm.IsSpeedCamLimitExceeded()),
    static_cast<jboolean>(rm.GetSpeedCamManager().ShouldPlayBeepSignal())
  );
  // clang-format on
  ASSERT(result, (jni::DescribeException()));
  return result;
}
