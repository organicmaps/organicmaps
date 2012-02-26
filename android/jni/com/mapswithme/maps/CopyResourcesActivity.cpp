#include <jni.h>
// To get free disk space
#include <sys/vfs.h>

#include "../../../../../defines.hpp"

#include "../../../../../base/logging.hpp"

#include "../../../../../coding/zip_reader.hpp"

#include "../../../../../platform/platform.hpp"

#include "../../../../../std/vector.hpp"
#include "../../../../../std/string.hpp"
#include "../../../../../std/bind.hpp"

// Special error codes to notify GUI about free space
//@{
#define ERR_COPIED_SUCCESSFULLY   0
#define ERR_NOT_ENOUGH_MEMORY     -1
#define ERR_NOT_ENOUGH_FREE_SPACE -2
#define ERR_STORAGE_DISCONNECTED  -3
//@}

struct FileToCopy
{
  string m_pathInZip;
  string m_pathInSdcard;
  uint64_t m_uncompressedSize;
};

static string g_apkPath;
static string g_sdcardPath;
static vector<FileToCopy> g_filesToCopy;
static jint g_copiedBytesProgress = 0;

extern "C"
{
  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_CopyResourcesActivity_nativeGetBytesToCopy(JNIEnv * env, jobject thiz,
      jstring apkPath, jstring sdcardPath)
  {
    {
      char const * strApkPath = env->GetStringUTFChars(apkPath, 0);
      if (!strApkPath)
        return ERR_NOT_ENOUGH_MEMORY;
      g_apkPath = strApkPath;
      env->ReleaseStringUTFChars(apkPath, strApkPath);

      char const * strSdcardPath = env->GetStringUTFChars(sdcardPath, 0);
      if (!strSdcardPath)
        return ERR_NOT_ENOUGH_MEMORY;
      g_sdcardPath = strSdcardPath;
      env->ReleaseStringUTFChars(sdcardPath, strSdcardPath);
    }

    jint totalSizeToCopy = 0;
    Platform::FilesList zipFiles = ZipFileReader::FilesList(g_apkPath);
    // Check which assets/* files are compressed and copy only these files
    string const ASSETS_PREFIX = "assets/";
    for (Platform::FilesList::iterator it = zipFiles.begin(); it != zipFiles.end(); ++it)
    {
      // Ignore non-assets files
      if (it->find(ASSETS_PREFIX) != 0)
        continue;

      uint64_t compressed, uncompressed;
      {
        ZipFileReader fileInZip(g_apkPath, *it);
        compressed = fileInZip.Size();
        uncompressed = fileInZip.UncompressedSize();
        // Skip files which are not compressed inside zip
        if (compressed == uncompressed)
          continue;
      }

      FileToCopy f;
      f.m_pathInSdcard = g_sdcardPath + it->substr(ASSETS_PREFIX.size());

      uint64_t realSizeInSdcard = 0;
      Platform::GetFileSizeByFullPath(f.m_pathInSdcard, realSizeInSdcard);
      if (uncompressed != realSizeInSdcard)
      {
        f.m_pathInZip = *it;
        f.m_uncompressedSize = uncompressed;
        g_filesToCopy.push_back(f);
        totalSizeToCopy += uncompressed;
      }
    }

    return totalSizeToCopy;
  }

  void CopyFileProgress(JNIEnv * env, jobject obj, jmethodID method, int size, int pos)
  {
    env->CallVoidMethod(obj, method, size, pos);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_CopyResourcesActivity_nativeCopyNextFile(JNIEnv * env,
      jobject thiz, jobject observer)
  {
    if (g_filesToCopy.empty())
      return ERR_COPIED_SUCCESSFULLY;

    vector<FileToCopy>::iterator it = g_filesToCopy.begin() + g_filesToCopy.size() - 1;
    // Check for free space available on the device
    struct statfs st;
    if (statfs(g_sdcardPath.c_str(), &st) != 0)
    {
      LOG(LWARNING, ("External filesystem is not available"));
      return ERR_STORAGE_DISCONNECTED;
    }
    if (st.f_bsize * st.f_bavail <= it->m_uncompressedSize)
    {
      LOG(LERROR, ("Not enough free space to extract file", it->m_pathInSdcard));
      return ERR_NOT_ENOUGH_FREE_SPACE;
    }

    jmethodID method = env->GetMethodID(env->GetObjectClass(observer), "onFileProgress", "(II)V");
    CHECK(method, ("Not existing method: void onFileProgress(int,int)"));

    // Perform copying
    try
    {
      ZipFileReader::UnzipFile(g_apkPath, it->m_pathInZip, it->m_pathInSdcard,
          bind(CopyFileProgress, env, observer, method, _1, _2));
    }
    catch (std::exception const & e)
    {
      LOG(LERROR, ("Error while extracting", it->m_pathInZip, "from apk to", it->m_pathInSdcard));
      return ERR_NOT_ENOUGH_FREE_SPACE;
    }

    g_copiedBytesProgress += it->m_uncompressedSize;
    g_filesToCopy.erase(it);
    return g_copiedBytesProgress;
  }

  // Move downloaded maps from /sdcard/Android/data/com.mapswithme.maps/files/ to /sdcard/MapsWithMe
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_CopyResourcesActivity_nativeMoveMaps(JNIEnv * env, jobject thiz,
      jstring fromPath, jstring toPath)
  {
    string from, to;
    {
      char const * strFrom = env->GetStringUTFChars(fromPath, 0);
      if (!strFrom)
        return;
      from = strFrom;
      env->ReleaseStringUTFChars(fromPath, strFrom);

      char const * strTo = env->GetStringUTFChars(toPath, 0);
      if (!strTo)
        return;
      to = strTo;
      env->ReleaseStringUTFChars(toPath, strTo);
    }

    Platform & pl = GetPlatform();
    Platform::FilesList files;
    // Move *.mwm files
    pl.GetFilesInDir(from, "*" DATA_FILE_EXTENSION, files);
    for (size_t i = 0; i < files.size(); ++i)
    {
      LOG(LINFO, (from + files[i], to + files[i]));
      rename((from + files[i]).c_str(), (to + files[i]).c_str());
    }

    // Delete not finished *.downloading files
    files.clear();
    pl.GetFilesInDir(from, "*" DOWNLOADING_FILE_EXTENSION, files);
    pl.GetFilesInDir(from, "*" RESUME_FILE_EXTENSION, files);
    for (size_t i = 0; i < files.size(); ++i)
      remove((from + files[i]).c_str());
  }
}
