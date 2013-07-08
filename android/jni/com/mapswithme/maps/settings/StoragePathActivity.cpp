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
  Java_com_mapswithme_maps_settings_StoragePathActivity_nativeSetStoragePath(JNIEnv * env, jobject thiz,
                                                                 jstring s)
  {
    string const from = GetPlatform().WritableDir();
    string const to = jni::ToNativeString(env, s);

    // Remove all maps from container.
    g_framework->RemoveLocalMaps();

    // Get files to copy.
    Platform & pl = GetPlatform();

    // Get regexp like this: (\.mwm$|\.ttf$)
    string const regexp = "(" "\\"DATA_FILE_EXTENSION"$" "|"
                              "\\"BOOKMARKS_FILE_EXTENSION"$" "|"
                              "\\"FONT_FILE_EXTENSION"$" ")";
    Platform::FilesList files;
    pl.GetFilesByRegExp(from, regexp, files);

    // Copy all needed files.
    for (size_t i = 0; i < files.size(); ++i)
      if (!my::CopyFileX(from + files[i], to + files[i]))
      {
        // Do the undo - delete all previously copied files.
        for (size_t j = 0; j <= i; ++j)
        {
          string const path = to + files[j];
          VERIFY ( my::DeleteFileX(path), (path) );
        }
        return false;
      }

    // Set new storage path.
    android::Platform::Instance().SetStoragePath(to);

    // Add all maps again.
    g_framework->AddLocalMaps();

    // Reload bookmarks again
    g_framework->NativeFramework()->LoadBookmarks();
    return true;
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_settings_SettingsActivity_isDownloadingActive(JNIEnv * env, jobject thiz)
  {
    return g_framework->IsDownloadingActive();
  }
}
