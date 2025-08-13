#include "LaneInfo.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include <vector>

namespace
{
jobject ToJavaLaneWay(JNIEnv * env, routing::turns::lanes::LaneWay const & laneWay)
{
  static jclass const laneWayClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/LaneWay");
  jfieldID fieldID =
      env->GetStaticFieldID(laneWayClass, DebugPrint(laneWay).c_str(), "Lapp/organicmaps/sdk/routing/LaneWay;");
  return env->GetStaticObjectField(laneWayClass, fieldID);
}
}  // namespace

jobjectArray CreateLanesInfo(JNIEnv * env, routing::turns::lanes::LanesInfo const & lanes)
{
  if (lanes.empty())
    return nullptr;

  static jclass const laneWayClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/LaneWay");
  static jclass const laneInfoClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/LaneInfo");
  auto const lanesSize = static_cast<jsize>(lanes.size());
  jobjectArray jLanes = env->NewObjectArray(lanesSize, laneInfoClass, nullptr);
  ASSERT(jLanes, (jni::DescribeException()));
  // Java signature : LaneInfo(LaneWay[] laneWays, LaneWay activeLane)
  static jmethodID const ctorLaneInfoID = jni::GetConstructorID(
      env, laneInfoClass, "([Lapp/organicmaps/sdk/routing/LaneWay;Lapp/organicmaps/sdk/routing/LaneWay;)V");

  for (jsize j = 0; j < lanesSize; ++j)
  {
    auto const laneWays = lanes[j].laneWays.GetActiveLaneWays();
    auto const laneWaysSize = static_cast<jsize>(laneWays.size());
    jni::TScopedLocalObjectArrayRef jLaneWays(env, env->NewObjectArray(laneWaysSize, laneWayClass, nullptr));
    ASSERT(jLanes, (jni::DescribeException()));
    for (jsize i = 0; i < laneWaysSize; ++i)
    {
      jni::TScopedLocalRef jLaneWay(env, ToJavaLaneWay(env, laneWays[i]));
      env->SetObjectArrayElement(jLaneWays.get(), i, jLaneWay.get());
    }
    jni::TScopedLocalRef jLaneInfo(env, env->NewObject(laneInfoClass, ctorLaneInfoID, jLaneWays.get(),
                                                       ToJavaLaneWay(env, lanes[j].recommendedWay)));
    ASSERT(jLaneInfo.get(), (jni::DescribeException()));
    env->SetObjectArrayElement(jLanes, j, jLaneInfo.get());
  }

  return jLanes;
}
