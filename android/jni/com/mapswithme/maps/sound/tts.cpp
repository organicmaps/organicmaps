#include "../Framework.hpp"

#include "../../core/jni_helper.hpp"


namespace
{
  ::Framework * frm() { return g_framework->NativeFramework(); }
} //  namespace

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_sound_TtsPlayer_nativeEnableTurnNotifications(JNIEnv * env, jclass thiz, jboolean enable)
  {
    return frm()->EnableTurnNotifications(static_cast<bool>(enable));
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_sound_TtsPlayer_nativeAreTurnNotificationsEnabled(JNIEnv * env, jclass clazz)
  {
    return static_cast<jboolean>(frm()->AreTurnNotificationsEnabled());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_sound_TtsPlayer_nativeSetTurnNotificationsLocale(JNIEnv * env, jclass thiz, jstring jLocale)
  {
    frm()->SetTurnNotificationsLocale(jni::ToNativeString(env, jLocale));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_sound_TtsPlayer_nativeGetTurnNotificationsLocale(JNIEnv * env, jclass thiz)
  {
    return jni::ToJavaString(env, frm()->GetTurnNotificationsLocale().c_str());
  }
} // extern "C"
