#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "routing/routing_options.hpp"

routing::RoutingOptions::Road makeValue(jint option)
{
  auto const road = static_cast<uint8_t>(1u << static_cast<int>(option));
  CHECK_LESS(road, static_cast<uint8_t>(routing::RoutingOptions::Road::Max), ());
  return static_cast<routing::RoutingOptions::Road>(road);
}

routing::EdgeEstimator::Strategy makeStrategyValue(jint strategy)
{
  int convertedStrat = static_cast<int>(strategy);
  return static_cast<routing::EdgeEstimator::Strategy>(convertedStrat);
}

extern "C"
{
JNIEXPORT jboolean Java_app_organicmaps_sdk_routing_RoutingOptions_nativeHasOption(JNIEnv *, jclass, jint option)
{
  routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadCarOptionsFromSettings();
  routing::RoutingOptions::Road road = makeValue(option);
  return static_cast<jboolean>(routingOptions.Has(road));
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_routing_RoutingOptions_nativeGetStrategy(JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  routing::EdgeEstimator::Strategy routingStrategy = routing::EdgeEstimator::LoadRoutingStrategyFromSettings();
  return static_cast<jint>(routingStrategy);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_routing_RoutingOptions_nativeAddOption(JNIEnv * env, jclass clazz, jint option)
{
  routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadCarOptionsFromSettings();
  routing::RoutingOptions::Road road = makeValue(option);
  routingOptions.Add(road);
  routing::RoutingOptions::SaveCarOptionsToSettings(routingOptions);
}

JNIEXPORT void Java_app_organicmaps_sdk_routing_RoutingOptions_nativeRemoveOption(JNIEnv *, jclass, jint option)
{
  routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadCarOptionsFromSettings();
  routing::RoutingOptions::Road road = makeValue(option);
  routingOptions.Remove(road);
  routing::RoutingOptions::SaveCarOptionsToSettings(routingOptions);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_routing_RoutingOptions_nativeSetStrategy(JNIEnv * env, jclass clazz, jint strategy)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  routing::EdgeEstimator::SaveRoutingStrategyToSettings(makeStrategyValue(strategy));
}
}
