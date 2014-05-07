#include "../Framework.hpp"
#include "../../core/jni_helper.hpp"


extern "C"
{
  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_settings_SettingsActivity_isDownloadingActive(JNIEnv * env, jobject thiz)
  {
    return g_framework->IsDownloadingActive();
  }
}
