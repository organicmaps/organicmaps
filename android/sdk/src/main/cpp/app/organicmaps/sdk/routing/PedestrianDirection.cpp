#include "PedestrianDirection.hpp"

namespace
{
std::string ToJavaPedestrianDirectionName(routing::turns::PedestrianDirection pedestrianDirection)
{
  if (pedestrianDirection == routing::turns::PedestrianDirection::None)
    return "NoTurn";
  return DebugPrint(pedestrianDirection);
}
}  // namespace

jobject ToJavaPedestrianDirection(JNIEnv * env, routing::turns::PedestrianDirection pedestrianDirection)
{
  static jclass const pedestrianDirectionClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/PedestrianDirection");
  jfieldID fieldID =
      env->GetStaticFieldID(pedestrianDirectionClass, ToJavaPedestrianDirectionName(pedestrianDirection).c_str(),
                            "Lapp/organicmaps/sdk/routing/PedestrianDirection;");
  return env->GetStaticObjectField(pedestrianDirectionClass, fieldID);
}
