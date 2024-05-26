#include "app/organicmaps/Framework.hpp"

#include "platform/settings.hpp"

extern "C"
{
  JNIEXPORT void JNICALL
  Java_app_organicmaps_settings_MapLanguageCode_setCurrentMapLanguageCode(JNIEnv * env, jobject, jstring jLanguageCode)
  {
    g_framework->SetMapLanguageCode(jni::ToNativeString(env, jLanguageCode));
  }

  JNIEXPORT jstring JNICALL
  Java_app_organicmaps_settings_MapLanguageCode_getCurrentMapLanguageCode(JNIEnv * env, jobject)
  {
    return jni::ToJavaString(env, g_framework->GetMapLanguageCode());
  }
}
