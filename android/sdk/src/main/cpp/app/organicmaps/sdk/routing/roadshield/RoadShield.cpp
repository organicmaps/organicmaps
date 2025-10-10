#include "RoadShield.hpp"

#include "app/organicmaps/sdk/routing/roadshield/RoadShieldType.hpp"

jobject ToJavaRoadShield(JNIEnv * env, ftypes::RoadShield const & roadShield)
{
  static jclass const klass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/roadshield/RoadShield");
  // clang-format off
  static jmethodID const ctorRouteInfoID = jni::GetConstructorID(env, klass,
    "("
    "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldType;"  // type
    "Ljava/lang/String;"                                       // text
    "Ljava/lang/String;"                                       // additionalText
    ")V"
  );
  // clang-format on

  // clang-format off
  jobject const result = env->NewObject(klass, ctorRouteInfoID,
    ToJavaRoadShieldType(env, roadShield.m_type),
    jni::ToJavaString(env, roadShield.m_name),
    jni::ToJavaString(env, roadShield.m_additionalText)
  );
  // clang-format on
  ASSERT(result, (jni::DescribeException()));
  return result;
}
