#include "../../core/jni_helper.hpp"
#include "../../platform/Platform.hpp"

#include "map/bookmark.hpp"
#include "std/string.hpp"


extern "C"
{

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_settings_StoragePathManager_nativeGenerateUniqueBookmarkName(JNIEnv * env, jclass thiz, jstring jBaseName)
{
  string baseName = jni::ToNativeString(env, jBaseName);
  string bookmarkFileName = BookmarkCategory::GenerateUniqueFileName(GetPlatform().SettingsDir(), baseName);
  return jni::ToJavaString(env, bookmarkFileName);
}

}
