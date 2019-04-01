#include <jni.h>
#include <android/jni/com/mapswithme/maps/Framework.hpp>
#include <android/jni/com/mapswithme/core/jni_helper.hpp>
#include "routing/routing_options.hpp"

unsigned int makeValue(jint option)
{
    return 1u << static_cast<int>(option);
}

extern "C"
{

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_routing_RoutingOptions_nativeHasOption(JNIEnv * env, jclass clazz, jint option)
{
    CHECK(g_framework, ("Framework isn't created yet!"));
    routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadCarOptionsFromSettings();
    routing::RoutingOptions::Road road = static_cast<routing::RoutingOptions::Road>(makeValue(option));
    return static_cast<jboolean>(routingOptions.Has(road));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_routing_RoutingOptions_nativeAddOption(JNIEnv * env, jclass clazz, jint option)
{
    CHECK(g_framework, ("Framework isn't created yet!"));
    routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadCarOptionsFromSettings();
    routing::RoutingOptions::Road road = static_cast<routing::RoutingOptions::Road>(makeValue(option));
    routingOptions.Add(road);
    routing::RoutingOptions::SaveCarOptionsToSettings(routingOptions);
}


JNIEXPORT void JNICALL
Java_com_mapswithme_maps_routing_RoutingOptions_nativeRemoveOption(JNIEnv * env, jclass clazz, jint option)
{
    CHECK(g_framework, ("Framework isn't created yet!"));
    routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadCarOptionsFromSettings();
    routing::RoutingOptions::Road road = static_cast<routing::RoutingOptions::Road>(makeValue(option));
    routingOptions.Remove(road);
    routing::RoutingOptions::SaveCarOptionsToSettings(routingOptions);
}
}
