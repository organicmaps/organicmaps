#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "routing/route_speed_settings.hpp"

extern "C"
{
JNIEXPORT jobject Java_app_organicmaps_sdk_routing_RouteSpeedSettings_nativeGet(JNIEnv * env, jclass)
{
  auto const vehicleType = frm()->GetRoutingManager().GetRouterVehicleType();
  if (!routing::IsRouteSpeedSupported(vehicleType))
    return nullptr;

  auto const settings = routing::LoadRouteSpeedSettings(vehicleType);
  auto const range = routing::GetCruisingSpeedRange(vehicleType);

  static jclass const clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RouteSpeedSettings");
  // Java signature : RouteSpeedSettings(double cruisingSpeedKmph, int windSpeedMps, int windDirectionDegrees,
  //                                     double minSpeedKmph, double maxSpeedKmph, double speedStepKmph,
  //                                     double defaultSpeedKmph, int maxWindSpeedMps)
  static jmethodID const constructor = jni::GetConstructorID(env, clazz, "(DIIDDDDI)V");
  return env->NewObject(clazz, constructor, static_cast<jdouble>(settings.m_cruisingSpeedKMpH),
                        static_cast<jint>(settings.m_windSpeedMpS), static_cast<jint>(settings.m_windDirectionDegrees),
                        static_cast<jdouble>(range.m_min), static_cast<jdouble>(range.m_max),
                        static_cast<jdouble>(range.m_step), static_cast<jdouble>(range.m_default),
                        static_cast<jint>(routing::IsWindSupported(vehicleType) ? routing::kMaxWindSpeedMpS : 0));
}

JNIEXPORT void Java_app_organicmaps_sdk_routing_RouteSpeedSettings_nativeSet(JNIEnv *, jclass,
                                                                             jdouble cruisingSpeedKmph,
                                                                             jint windSpeedMps,
                                                                             jint windDirectionDegrees)
{
  frm()->GetRoutingManager().SetRouteSpeedSettings({cruisingSpeedKmph, windSpeedMps, windDirectionDegrees});
}
}
