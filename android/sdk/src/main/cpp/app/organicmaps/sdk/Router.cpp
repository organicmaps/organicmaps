#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

#include "app/organicmaps/sdk/util/Distance.hpp"
#include "indexer/map_style.hpp"

extern "C"
{
JNIEXPORT void Java_app_organicmaps_sdk_Router_nativeSet(JNIEnv *, jclass, jint routerType)
{
  using Type = routing::RouterType;
  Type type;
  switch (routerType)
  {
  case 0: type = Type::Vehicle; break;
  case 1: type = Type::Pedestrian; break;
  case 2: type = Type::Bicycle; break;
  case 3: type = Type::Transit; break;
  case 4: type = Type::Ruler; break;
  case 5: type = Type::Water; break;
  default: ASSERT(false, (routerType)); return;
  }
  frm()->GetRoutingManager().SetRouter(type);
}

JNIEXPORT jint Java_app_organicmaps_sdk_Router_nativeGet(JNIEnv *, jclass)
{
  return static_cast<jint>(frm()->GetRoutingManager().GetRouter());
}

JNIEXPORT jint Java_app_organicmaps_sdk_Router_nativeGetLastUsed(JNIEnv *, jclass)
{
  return static_cast<jint>(frm()->GetRoutingManager().GetLastUsedRouter());
}

JNIEXPORT jint Java_app_organicmaps_sdk_Router_nativeGetBest(JNIEnv *, jclass, jdouble srcLat, jdouble srcLon,
                                                             jdouble dstLat, jdouble dstLon)
{
  return static_cast<jint>(frm()->GetRoutingManager().GetBestRouter(mercator::FromLatLon(srcLat, srcLon),
                                                                    mercator::FromLatLon(dstLat, dstLon)));
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_routing_WaterRouting_isShortestConfirmed(JNIEnv *, jclass)
{
  auto const distances = frm()->GetRoutingManager().GetWaterDistances();
  return distances && distances->m_shortestConfirmed;
}

JNIEXPORT jobjectArray Java_app_organicmaps_sdk_routing_WaterRouting_getDistances(JNIEnv * env, jclass)
{
  auto const distances = frm()->GetRoutingManager().GetWaterDistances();
  if (!distances)
    return nullptr;
  auto result = env->NewObjectArray(3, jni::GetGlobalClassRef(env, "app/organicmaps/sdk/util/Distance"), nullptr);
  size_t i = 0;
  for (auto const meters : {distances->m_waterMeters, distances->m_landMeters, distances->GetTotalMeters()})
  {
    auto const value = ToJavaDistance(env, platform::Distance::CreateFormatted(meters));
    env->SetObjectArrayElement(result, i++, value);
    env->DeleteLocalRef(value);
  }
  return result;
}
}
