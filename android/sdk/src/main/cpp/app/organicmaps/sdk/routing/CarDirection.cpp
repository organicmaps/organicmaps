#include "CarDirection.hpp"

using routing::turns::CarDirection;

jobject ToJavaCarDirection(JNIEnv * env, CarDirection carDirection)
{
  static jclass const carDirectionClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/CarDirection");
  jfieldID fieldID = env->GetStaticFieldID(carDirectionClass, DebugPrint(carDirection).c_str(),
                                           "Lapp/organicmaps/sdk/routing/CarDirection;");
  return env->GetStaticObjectField(carDirectionClass, fieldID);
}
