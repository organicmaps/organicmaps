#include "../Framework.hpp"

#include "../../core/jni_helper.hpp"

#include "../../platform/Platform.hpp"

#include "../../../../../../coding/internal/file_data.hpp"


extern "C"
{
  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_settings_StoragePathActivity_nativeGetStoragePath(JNIEnv * env, jobject thiz)
  {
    return jni::ToJavaString(env, android::Platform::Instance().GetStoragePathPrefix());
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_settings_SettingsActivity_isDownloadingActive(JNIEnv * env, jobject thiz)
  {
    return g_framework->IsDownloadingActive();
  }
}
