#include "com/mapswithme/maps/Framework.hpp"

#include "com/mapswithme/core/jni_helper.hpp"


namespace
{
  ::Framework * frm() { return g_framework->NativeFramework(); }
} //  namespace

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_sound_TtsPlayer_nativeEnableTurnNotifications(JNIEnv *, jclass, jboolean enable)
  {
    return frm()->GetRoutingManager().EnableTurnNotifications(static_cast<bool>(enable));
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_sound_TtsPlayer_nativeAreTurnNotificationsEnabled(JNIEnv *, jclass)
  {
    return static_cast<jboolean>(frm()->GetRoutingManager().AreTurnNotificationsEnabled());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_sound_TtsPlayer_nativeSetTurnNotificationsLocale(JNIEnv * env, jclass, jstring jLocale)
  {
    frm()->GetRoutingManager().SetTurnNotificationsLocale(jni::ToNativeString(env, jLocale));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_sound_TtsPlayer_nativeGetTurnNotificationsLocale(JNIEnv * env, jclass)
  {
    return jni::ToJavaString(env, frm()->GetRoutingManager().GetTurnNotificationsLocale());
  }
} // extern "C"
