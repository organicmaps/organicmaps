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
  std::vector<std::pair<double, int>> allPoints;
  info.ForEachPoint([&](double d, geometry::Altitude a) { allPoints.emplace_back(d, a); });

  CHECK(!allPoints.empty(), ("Elevation points must be non empty!"));
  return jni::ToJavaArray(env, GetElevationPointClass(env), allPoints,
                          [](JNIEnv * env, std::pair<double, int> const & item)
  { return CreateElevationPoint(env, item.first, item.second); });
}

jdoubleArray ToSegmentDistancesArray(JNIEnv * env, ElevationInfo const & info)
{
  auto const & lines = info.GetLines();
  if (lines.empty())
    return env->NewDoubleArray(0);

  std::vector<double> breaks;
  breaks.reserve(lines.size() - 1);
  double const totalLength = info.GetLength();
  double cumulativeOffset = 0;
  for (size_t i = 0; i < lines.size(); ++i)
  {
    if (i > 0 && cumulativeOffset > 0 && cumulativeOffset < totalLength)
      breaks.emplace_back(cumulativeOffset);
    if (!lines[i].empty())
      cumulativeOffset += lines[i].back().m_distance;
  }

  jdoubleArray result = env->NewDoubleArray(static_cast<jsize>(breaks.size()));
  if (!breaks.empty())
    env->SetDoubleArrayRegion(result, 0, static_cast<jsize>(breaks.size()), breaks.data());
  return result;
}
}  // namespace

jobject ToJavaElevationInfo(JNIEnv * env, ElevationInfo const & info)
{
  // public ElevationInfo(@NonNull Point[] points, int difficulty, @NonNull double[] segmentDistances)
  static jmethodID const ctorId = jni::GetConstructorID(
      env, g_elevationInfoClazz, "([Lapp/organicmaps/sdk/bookmarks/data/ElevationInfo$Point;I[D)V");

  jni::TScopedLocalObjectArrayRef jPoints(env, ToElevationPointArray(env, info));
  jni::TScopedLocalRef jSegDist(env, ToSegmentDistancesArray(env, info));
  return env->NewObject(g_elevationInfoClazz, ctorId, jPoints.get(), static_cast<jint>(info.GetDifficulty()),
                        jSegDist.get());
}
