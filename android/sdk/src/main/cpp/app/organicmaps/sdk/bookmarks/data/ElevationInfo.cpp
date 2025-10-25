#include "Track.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

namespace
{
jobject CreateElevationPoint(JNIEnv * env, ElevationInfo::Point const & point)
{
  static jclass const pointClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/ElevationInfo$Point");
  // public Point(double distance, int altitude, double latitude, double longitude)
  static jmethodID const pointCtorId = jni::GetConstructorID(env, pointClass, "(DIDD)V");
  return env->NewObject(
      pointClass, pointCtorId, static_cast<jdouble>(point.m_distance), static_cast<jint>(point.m_point.GetAltitude()),
      static_cast<jdouble>(point.m_point.GetPoint().x), static_cast<jdouble>(point.m_point.GetPoint().y));
}

jobjectArray ToElevationPointArray(JNIEnv * env, ElevationInfo::Points const & points)
{
  CHECK(!points.empty(), ("Elevation points must be non empty!"));
  static jclass const pointClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/ElevationInfo$Point");
  return jni::ToJavaArray(env, pointClass, points, [](JNIEnv * env, ElevationInfo::Point const & item)
  { return CreateElevationPoint(env, item); });
}
}  // namespace

jobject ToJavaElevationInfoPoint(JNIEnv * env, ms::LatLon const & latlon)
{
  static jclass const clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/ElevationInfo$Point");
  static jmethodID const ctorId = jni::GetConstructorID(env, clazz, "(DIDD)V");
  return env->NewObject(clazz, ctorId, 0.0, 0, latlon.m_lat, latlon.m_lon);
}

jobject ToJavaElevationInfo(JNIEnv * env, ElevationInfo const & info)
{
  // public ElevationInfo(@NonNull Point[] points, int difficulty);
  static jmethodID const ctorId =
      jni::GetConstructorID(env, g_elevationInfoClazz, "([Lapp/organicmaps/sdk/bookmarks/data/ElevationInfo$Point;I)V");

  jni::TScopedLocalObjectArrayRef jPoints(env, ToElevationPointArray(env, info.GetPoints()));
  return env->NewObject(g_elevationInfoClazz, ctorId, jPoints.get(), static_cast<jint>(info.GetDifficulty()));
}
