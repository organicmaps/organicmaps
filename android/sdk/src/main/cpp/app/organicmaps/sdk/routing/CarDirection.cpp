#include "CarDirection.hpp"

namespace
{
std::string ToJavaCarDirectionName(routing::turns::CarDirection carDirection)
{
  if (carDirection == routing::turns::CarDirection::None)
    return "NoTurn";
  return DebugPrint(carDirection);
}
}  // namespace

jobject ToJavaCarDirection(JNIEnv * env, routing::turns::CarDirection carDirection)
{
  static jclass const carDirectionClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/CarDirection");
  jfieldID fieldID = env->GetStaticFieldID(carDirectionClass, ToJavaCarDirectionName(carDirection).c_str(),
                                           "Lapp/organicmaps/sdk/routing/CarDirection;");
  return env->GetStaticObjectField(carDirectionClass, fieldID);
}
