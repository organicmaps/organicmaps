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

    g_framework->RemoveLocalMaps();

    Platform & pl = GetPlatform();
    char const * arrExt[] = { DATA_FILE_EXTENSION, ".ttf", ".ini" };

    for (size_t i = 0; i < ARRAY_SIZE(arrExt); ++i)
    {
      Platform::FilesList files;
      pl.GetFilesInDir(from, "*" DATA_FILE_EXTENSION, files);

      for (size_t j = 0; j < files.size(); ++j)
        if (!my::CopyFile((from + files[j]).c_str(), (to + files[j]).c_str()))
          return false;
    }

    /// @todo Delete old folder.

    android::Platform::Instance().SetStoragePath(to);

    g_framework->AddLocalMaps();
    return true;
  }
}
