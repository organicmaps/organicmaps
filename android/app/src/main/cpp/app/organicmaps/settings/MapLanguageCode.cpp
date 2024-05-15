#include "app/organicmaps/Framework.hpp"

#include "platform/settings.hpp"

extern "C"
{
  JNIEXPORT void JNICALL
  Java_app_organicmaps_settings_MapLanguageCode_setCurrentMapLanguageCode(JNIEnv * env, jobject, jstring jLanguageCode)
  {
    std::string newLanguageCode = jni::ToNativeString(env, jLanguageCode);
    languages::SetMapLanguageCode(newLanguageCode);
    g_framework->SetMapLanguageCode(newLanguageCode);
  }

  JNIEXPORT jstring JNICALL
  Java_app_organicmaps_settings_MapLanguageCode_getCurrentMapLanguageCode(JNIEnv * env, jobject)
  {
    return jni::ToJavaString(env, languages::GetCurrentMapLanguageCode());
  }
}
