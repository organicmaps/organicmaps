#include "RoadShieldInfo.hpp"

#include "app/organicmaps/sdk/routing/roadshield/RoadShield.hpp"

namespace
{
jobjectArray ToJavaRoadShieldsArray(JNIEnv * env, ftypes::RoadShieldsSetT const & roadShields)
{
  static jclass const roadShieldClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/roadshield/RoadShield");
  return jni::ToJavaArray(env, roadShieldClass, roadShields, ToJavaRoadShield);
}
}  // namespace

jobject ToJavaRoadShieldInfo(JNIEnv * env, routing::FollowingInfo::RoadShieldInfo const & roadShieldInfo)
{
  static jclass const klass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/roadshield/RoadShieldInfo");
  // clang-format off
  static jmethodID const ctorRouteInfoID = jni::GetConstructorID(env, klass,
    "("
    "[Lapp/organicmaps/sdk/routing/roadshield/RoadShield;"  // targetRoadShields
    "I"                                                     // targetRoadShieldsIndexStart
    "I"                                                     // targetRoadShieldsIndexEnd
    "[Lapp/organicmaps/sdk/routing/roadshield/RoadShield;"  // junctionShields
    "I"                                                     // junctionShieldsIndexStart
    "I"                                                     // junctionShieldsIndexEnd
    ")V"
  );
  // clang-format on

  if (roadShieldInfo.m_targetRoadShields.empty() && roadShieldInfo.m_junctionShields.empty())
    return nullptr;

  // clang-format off
  jobject const result = env->NewObject(klass, ctorRouteInfoID,
    roadShieldInfo.m_targetRoadShields.empty() ? nullptr : ToJavaRoadShieldsArray(env, roadShieldInfo.m_targetRoadShields),
    roadShieldInfo.m_targetRoadShieldsPosition.first,
    roadShieldInfo.m_targetRoadShieldsPosition.second,
    roadShieldInfo.m_junctionShields.empty() ? nullptr : ToJavaRoadShieldsArray(env, roadShieldInfo.m_junctionShields),
    roadShieldInfo.m_junctionShieldsPosition.first,
    roadShieldInfo.m_junctionShieldsPosition.second
  );
  // clang-format on
  ASSERT(result, (jni::DescribeException()));
  return result;
}
