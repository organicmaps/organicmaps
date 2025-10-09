#include "PedestrianDirection.hpp"

namespace
{
std::string ToJavaPedestrianTurnDirectionName(routing::turns::PedestrianDirection pedestrianDirection)
{
  if (pedestrianDirection == routing::turns::PedestrianDirection::None)
    return "NoTurn";
  return DebugPrint(pedestrianDirection);
}
}  // namespace

jobject ToJavaPedestrianTurnDirection(JNIEnv * env, routing::turns::PedestrianDirection pedestrianDirection)
{
  static jclass const pedestrianDirectionClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/PedestrianTurnDirection");
  jfieldID fieldID =
      env->GetStaticFieldID(pedestrianDirectionClass, ToJavaPedestrianTurnDirectionName(pedestrianDirection).c_str(),
                            "Lapp/organicmaps/sdk/routing/PedestrianTurnDirection;");
  return env->GetStaticObjectField(pedestrianDirectionClass, fieldID);
}
