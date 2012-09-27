#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "../platform/Platform.hpp"

#include "../../../../../coding/internal/file_data.hpp"


extern "C"
{
  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_SettingsActivity_nativeGetStoragePath(JNIEnv * env, jobject thiz)
  {
    return jni::ToJavaString(env, android::Platform::Instance().GetStoragePathPrefix());
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_SettingsActivity_nativeSetStoragePath(JNIEnv * env, jobject thiz,
                                                                 jstring s)
  {
    string const from = GetPlatform().WritableDir();
    string const to = jni::ToNativeString(env, s);

    // Remove all maps from container.
    g_framework->RemoveLocalMaps();

    Platform & pl = GetPlatform();
    char const * arrMask[] = { "*" DATA_FILE_EXTENSION, "*.ttf" };

    // Copy all needed files.
    for (size_t i = 0; i < ARRAY_SIZE(arrMask); ++i)
    {
      Platform::FilesList files;
      pl.GetFilesInDir(from, arrMask[i], files);

      for (size_t j = 0; j < files.size(); ++j)
        if (!my::CopyFile((from + files[j]).c_str(), (to + files[j]).c_str()))
          return false;
    }

    // Set new storage path.
    android::Platform::Instance().SetStoragePath(to);

    // Add all maps again.
    g_framework->AddLocalMaps();
    return true;
  }
}
