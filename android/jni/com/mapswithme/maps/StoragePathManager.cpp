/*
 * StoragePathManager.cpp
 *
 *  Created on: 23 ???. 2014 ?.
 *      Author: ExMix
 */
#include "../../platform/Platform.hpp"
#include "../core/jni_helper.hpp"
#include "Framework.hpp"

#include "../../../../../map/bookmark.hpp"
#include "../../../../../base/stl_add.hpp"
#include "../../../../../coding/file_name_utils.hpp"
#include "../../../../../coding/internal/file_data.hpp"
#include "../../../../../std/set.hpp"
#include "../../../../../std/string.hpp"
#include "../../../../../std/algorithm.hpp"

namespace
{
  struct PathInserter
  {
  public:
    PathInserter(string const & dirPath, set<string> & set)
      : m_dirPath(dirPath)
      , m_set(set)
    {
      if (m_dirPath[m_dirPath.size() - 1] != '/')
        m_dirPath = m_dirPath + "/";
    }

    void operator() (string const & name)
    {
      m_set.insert(m_dirPath + "/" + name);
    }

  private:
    string m_dirPath;
    set<string> & m_set;
  };
}

extern "C"
{

JNIEXPORT jstring JNICALL
Java_com_mapswithme_util_StoragePathManager_nativeGetBookmarkDir(JNIEnv * env, jclass thiz)
{
  return jni::ToJavaString(env, GetPlatform().SettingsDir().c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_util_StoragePathManager_nativeMoveBookmarks(JNIEnv * env, jclass thiz, jobjectArray pathArray, jlong storageAvSize)
{
  set<string> fullBookmarkSet;
  Platform * pl = &GetPlatform();
  string settingsDir = pl->SettingsDir();
  string writableDir = pl->WritableDir();
  if (writableDir != settingsDir)
  {
    Platform::FilesList list;
    pl->GetFilesByExt(writableDir, BOOKMARKS_FILE_EXTENSION, list);
    for_each(list.begin(), list.end(), PathInserter(writableDir, fullBookmarkSet));
  }

  int arraySize = env->GetArrayLength(pathArray);
  for (int i = 0; i < arraySize; ++i)
  {
    jstring jPath = (jstring)env->GetObjectArrayElement(pathArray, i);
    string path = jni::ToNativeString(env, jPath);

    if (path != settingsDir)
    {
      Platform::FilesList list;
      pl->GetFilesByExt(path, BOOKMARKS_FILE_EXTENSION, list);
      for_each(list.begin(), list.end(), PathInserter(path, fullBookmarkSet));
    }
  }

  typedef set<string>::const_iterator fileIt;

  uint64_t fullSize = 0;
  for (fileIt it = fullBookmarkSet.begin(); it != fullBookmarkSet.end(); ++it)
  {
    uint64_t size = 0;
    my::GetFileSize(*it, size);
    fullSize += size;
  }

  if (storageAvSize < fullSize)
    return false;

  for (fileIt it = fullBookmarkSet.begin(); it != fullBookmarkSet.end(); ++it)
  {
    string oldFilePath = *it;
    string fileName = oldFilePath;
    my::GetNameFromFullPath(fileName);
    my::GetNameWithoutExt(fileName);
    string newFilePath = BookmarkCategory::GenerateUniqueFileName(settingsDir, fileName);

    if (my::CopyFileX(oldFilePath, newFilePath))
      my::DeleteFileX(oldFilePath);
  }

  g_framework->NativeFramework()->LoadBookmarks();
  return true;
}

}
