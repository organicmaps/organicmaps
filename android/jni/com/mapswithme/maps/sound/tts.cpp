#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"


namespace
{
  ::Framework * frm() { return g_framework->NativeFramework(); }
} //  namespace

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_sound_TTSPlayer_nativeEnableTurnNotifications(JNIEnv * env, jclass thiz, jboolean enable)
  {
    return frm()->EnableTurnNotifications(enable == JNI_TRUE ? true : false);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_sound_TTSPlayer_nativeAreTurnNotificationsEnabled(JNIEnv * env, jclass clazz)
  {
    return frm()->AreTurnNotificationsEnabled() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_sound_TTSPlayer_nativeSetTurnNotificationsLocale(JNIEnv * env, jclass thiz, jstring jLocale)
  {
    frm()->SetTurnNotificationsLocale(jni::ToNativeString(env, jLocale));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_sound_TTSPlayer_nativeGetTurnNotificationsLocale(JNIEnv * env, jclass thiz)
  {
    return jni::ToJavaString(env, frm()->GetTurnNotificationsLocale().c_str());
  }
} // extern "C"