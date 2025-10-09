#include "PedestrianDirection.hpp"

using routing::turns::PedestrianDirection;

jobject ToJavaPedestrianTurnDirection(JNIEnv * env, PedestrianDirection pedestrianDirection)
{
  static jclass const pedestrianDirectionClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/PedestrianTurnDirection");
  jfieldID fieldID = env->GetStaticFieldID(pedestrianDirectionClass, DebugPrint(pedestrianDirection).c_str(),
                                           "Lapp/organicmaps/sdk/routing/PedestrianTurnDirection;");
  return env->GetStaticObjectField(pedestrianDirectionClass, fieldID);
}
