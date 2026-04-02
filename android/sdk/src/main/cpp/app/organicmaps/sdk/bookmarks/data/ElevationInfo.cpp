#include "Track.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

namespace
{
jclass GetElevationPointClass(JNIEnv * env)
{
  static jclass const pointClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/ElevationInfo$Point");
  return pointClass;
}

jobject CreateElevationPoint(JNIEnv * env, double distance, int altitude)
{
  // public Point(double distance, int altitude)
  static jmethodID const pointCtorId = jni::GetConstructorID(env, GetElevationPointClass(env), "(DI)V");
  return env->NewObject(GetElevationPointClass(env), pointCtorId, static_cast<jdouble>(distance),
                        static_cast<jint>(altitude));
}

jobjectArray ToElevationPointArray(JNIEnv * env, ElevationInfo const & info)
{
  // Flatten all lines into a single array with cumulative distances.
  std::vector<std::pair<double, int>> allPoints;
  double cumulativeOffset = 0;
  for (auto const & line : info.GetLines())
  {
    for (auto const & point : line)
      allPoints.emplace_back(cumulativeOffset + point.m_distance, point.m_altitude);

    if (!line.empty())
      cumulativeOffset += line.back().m_distance;
  }

  CHECK(!allPoints.empty(), ("Elevation points must be non empty!"));
  return jni::ToJavaArray(env, GetElevationPointClass(env), allPoints,
                          [](JNIEnv * env, std::pair<double, int> const & item)
  { return CreateElevationPoint(env, item.first, item.second); });
}
}  // namespace

jobject ToJavaElevationInfo(JNIEnv * env, ElevationInfo const & info)
{
  // public ElevationInfo(@NonNull Point[] points, int difficulty);
  static jmethodID const ctorId =
      jni::GetConstructorID(env, g_elevationInfoClazz, "([Lapp/organicmaps/sdk/bookmarks/data/ElevationInfo$Point;I)V");

  jni::TScopedLocalObjectArrayRef jPoints(env, ToElevationPointArray(env, info));
  return env->NewObject(g_elevationInfoClazz, ctorId, jPoints.get(), static_cast<jint>(info.GetDifficulty()));
}
