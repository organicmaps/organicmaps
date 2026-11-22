#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

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
  case 5: type = Type::Motorcycle; break;
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

JNIEXPORT void Java_app_organicmaps_sdk_Router_nativeSetMotorcycleProfile(JNIEnv *, jclass, jint profile)
{
  using Profile = routing::MotorcycleProfile;
  Profile mode;
  switch (profile)
  {
  case 0: mode = Profile::Default; break;
  case 1: mode = Profile::Fun; break;
  case 2: mode = Profile::Offroad; break;
  case 3: mode = Profile::HardEnduro; break;
  case 4: mode = Profile::Touring; break;
  default: ASSERT(false, (profile)); return;
  }
  frm()->GetRoutingManager().SetMotorcycleProfile(mode);
}

JNIEXPORT jint Java_app_organicmaps_sdk_Router_nativeGetMotorcycleProfile(JNIEnv *, jclass)
{
  return static_cast<jint>(frm()->GetRoutingManager().GetMotorcycleProfile());
}
}
