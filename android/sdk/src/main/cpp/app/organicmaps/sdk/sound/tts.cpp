#include "app/organicmaps/sdk/Framework.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "platform/languages.hpp"

extern "C"
{
JNIEXPORT void Java_app_organicmaps_sdk_sound_TtsPlayer_nativeEnableTurnNotifications(JNIEnv *, jclass, jboolean enable)
{
  return frm()->GetRoutingManager().EnableTurnNotifications(static_cast<bool>(enable));
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_sound_TtsPlayer_nativeAreTurnNotificationsEnabled(JNIEnv *, jclass)
{
  return static_cast<jboolean>(frm()->GetRoutingManager().AreTurnNotificationsEnabled());
}

JNIEXPORT void Java_app_organicmaps_sdk_sound_TtsPlayer_nativeSetTurnNotificationsLocale(JNIEnv * env, jclass,
                                                                                         jstring jLocale)
{
  frm()->GetRoutingManager().SetTurnNotificationsLocale(jni::ToNativeString(env, jLocale));
}

JNIEXPORT jstring Java_app_organicmaps_sdk_sound_TtsPlayer_nativeGetTurnNotificationsLocale(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, frm()->GetRoutingManager().GetTurnNotificationsLocale());
}

JNIEXPORT jobject Java_app_organicmaps_sdk_sound_TtsPlayer_nativeGetSupportedLanguages(JNIEnv * env, jclass)
{
  auto const & supportedLanguages = routing::turns::sound::kLanguageList;

  auto const & listBuilder = jni::ListBuilder::Instance(env);
  jobject const list = listBuilder.CreateArray(env, supportedLanguages.size());
  for (auto const & [lang, name] : supportedLanguages)
  {
    jni::TScopedLocalRef const jLangString(env, jni::ToJavaString(env, lang));
    jni::TScopedLocalRef const jNameString(env, jni::ToJavaString(env, name));
    jni::TScopedLocalRef const pair(env,
                                    jni::PairBuilder::Instance(env).Create(env, jLangString.get(), jNameString.get()));
    env->CallBooleanMethod(list, listBuilder.m_add, pair.get());
  }
  return list;
}
}  // extern "C"
