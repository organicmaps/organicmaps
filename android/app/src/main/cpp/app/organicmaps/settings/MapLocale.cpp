#include "app/organicmaps/Framework.hpp"

#include "platform/settings.hpp"

extern "C"
{
  JNIEXPORT void JNICALL
  Java_app_organicmaps_settings_MapLocale_setCurrentMapLocale(JNIEnv * env, jobject, jstring jLocale)
  {
    std::string newLocale = jni::ToNativeString(env, jLocale);
    
    settings::Set(settings::kMapLocale, newLocale);
    g_framework->SaveMapLocale(newLocale);
    g_framework->SetMapLocale(newLocale);
  }

  JNIEXPORT jstring JNICALL
  Java_app_organicmaps_settings_MapLocale_getCurrentMapLocale(JNIEnv * env, jobject)
  {
    std::string sLocale;
    settings::Get(settings::kMapLocale, sLocale);
    if (sLocale.empty())
        sLocale = languages::GetCurrentNorm(); 
    
    return jni::ToJavaString(env, sLocale);
  }

  JNIEXPORT jstring JNICALL
  Java_app_organicmaps_settings_MapLocale_getDefaultMapLocale(JNIEnv * env, jobject)
  { 
    return jni::ToJavaString(env, languages::GetCurrentNorm());
  }
}
