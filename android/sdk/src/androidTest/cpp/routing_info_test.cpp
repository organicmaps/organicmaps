#include "app/organicmaps/sdk/routing/RoutingJni.hpp"

extern "C"
{
JNIEXPORT jobject Java_app_organicmaps_sdk_routing_RoutingInfoTest_nativeCreateFollowingInfo(
    JNIEnv * env, jclass, jint seconds, jboolean speedCamLimitExceeded, jboolean shouldPlayWarningSignal)
{
  routing::FollowingInfo info;
  info.m_time = seconds;
  info.m_distToTarget = platform::Distance(100.0, platform::Distance::Units::Meters);
  info.m_distToTurn = platform::Distance(10.0, platform::Distance::Units::Meters);
  return routing_jni::CreateRoutingInfo(env, info, speedCamLimitExceeded, shouldPlayWarningSignal);
}

JNIEXPORT jobject Java_app_organicmaps_sdk_routing_RoutingInfoTest_nativeCreateTransitInfo(JNIEnv * env, jclass,
                                                                                           jint seconds)
{
  TransitRouteInfo info;
  info.m_totalTimeInSec = seconds;
  info.AddStep(TransitStepInfo(TransitType::Pedestrian, 42.0, 21));
  return routing_jni::CreateTransitRouteInfo(env, info);
}
}
