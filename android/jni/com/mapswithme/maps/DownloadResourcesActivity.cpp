#include <jni.h>
// To get free disk space
#include <sys/vfs.h>

#include "../../../../../defines.hpp"

#include "../../../../../platform/http_request.hpp"

#include "../../../../../base/logging.hpp"
#include "../../../../../base/string_utils.hpp"

#include "../../../../../coding/zip_reader.hpp"
#include "../../../../../coding/url_encode.hpp"

#include "../../../../../platform/platform.hpp"
#include "../../../../../platform/http_request.hpp"

#include "../../../../../std/vector.hpp"
#include "../../../../../std/string.hpp"
#include "../../../../../std/bind.hpp"

#include "../core/jni_helper.hpp"

#include "Framework.hpp"


// Special error codes to notify GUI about free space
//@{
#define ERR_DOWNLOAD_SUCCESS   0
#define ERR_NOT_ENOUGH_MEMORY     -1
#define ERR_NOT_ENOUGH_FREE_SPACE -2
#define ERR_STORAGE_DISCONNECTED  -3
#define ERR_DOWNLOAD_ERROR -4
#define ERR_NO_MORE_FILES -5
#define ERR_FILE_IN_PROGRESS -6
//@}

struct FileToDownload
{
  vector<string> m_urls;
  string m_fileName;
  string m_pathOnSdcard;
  uint64_t m_fileSize;
};

static string g_apkPath;
static string g_sdcardPath;
static vector<FileToDownload> g_filesToDownload;
static int g_totalDownloadedBytes;
static int g_totalBytesToDownload;

extern "C"
{
  int HasSpaceForFiles(size_t fileSize)
  {
    struct statfs st;

    if (statfs(g_sdcardPath.c_str(), &st) != 0)
      return ERR_STORAGE_DISCONNECTED;

    if (st.f_bsize * st.f_bavail <= fileSize)
      return ERR_NOT_ENOUGH_FREE_SPACE;

    return fileSize;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadResourcesActivity_nativeGetBytesToDownload(JNIEnv * env, jobject thiz,
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

    jint totalBytesToDownload = 0;

    string buffer;
    ReaderPtr<Reader>(GetPlatform().GetReader("external_resources.txt")).ReadAsString(buffer);

    stringstream in(buffer);

    string name;
    int size;

    while (true)
    {
      in >> name;

      if (!in.good())
        break;

      in >> size;

      if (!in.good())
        break;

      FileToDownload f;

      f.m_pathOnSdcard = g_sdcardPath + name;
      f.m_fileName = name;

      uint64_t sizeOnSdcard = 0;
      Platform::GetFileSizeByFullPath(f.m_pathOnSdcard, sizeOnSdcard);

      if (size != sizeOnSdcard)
      {
        LOG(LINFO, ("should check : ", name, "sized", size, "bytes"));
        f.m_fileSize = size;
        g_filesToDownload.push_back(f);
        totalBytesToDownload += size;
      }
    }

    g_totalDownloadedBytes = 0;

    int res = HasSpaceForFiles(totalBytesToDownload);

    switch (res)
    {
    case ERR_STORAGE_DISCONNECTED:
      LOG(LWARNING, ("External file system is not available"));
      break;
    case ERR_NOT_ENOUGH_FREE_SPACE:
      LOG(LWARNING, ("Not enough space to extract files"));
      break;
    };

    g_totalBytesToDownload = totalBytesToDownload;

    return res;
  }

  void DownloadFileFinished(shared_ptr<jobject> obj, downloader::HttpRequest & req)
  {
    int errorCode = 0;

    CHECK(req.Status() != downloader::HttpRequest::EInProgress, ("should be either Completed or Failed"));

    switch (req.Status())
    {
    case downloader::HttpRequest::ECompleted:
      errorCode = ERR_DOWNLOAD_SUCCESS;
      break;
    case downloader::HttpRequest::EFailed:
      errorCode = ERR_DOWNLOAD_ERROR;
      break;
    };

    FileToDownload & curFile = g_filesToDownload.back();

    LOG(LINFO, ("finished downloading", curFile.m_fileName, "sized", curFile.m_fileSize, "bytes"));

    /// slight hack, check manually for Maps extension and AddMap accordingly
    if (curFile.m_fileName.find(".mwm") != string::npos)
    {
      LOG(LINFO, ("adding it as a map"));
      g_framework->AddMap(curFile.m_fileName);
    }

    g_totalDownloadedBytes += curFile.m_fileSize;

    LOG(LINFO, ("totalDownloadedBytes:", g_totalDownloadedBytes));

    g_filesToDownload.pop_back();

    JNIEnv * env = jni::GetEnv();

    jmethodID onFinishMethod = env->GetMethodID(env->GetObjectClass(*obj.get()), "onDownloadFinished", "(I)V");
    CHECK(onFinishMethod, ("Not existing method: void onDownloadFinished(int)"));

    env->CallVoidMethod(*obj.get(), onFinishMethod, errorCode);
  }

  void DownloadFileProgress(shared_ptr<jobject> obj, downloader::HttpRequest & req)
  {
    LOG(LINFO, (req.Progress().first, "bytes for", g_filesToDownload.back().m_fileName, "was downloaded"));

    JNIEnv * env = jni::GetEnv();

    jmethodID onProgressMethod = env->GetMethodID(env->GetObjectClass(*obj.get()), "onDownloadProgress", "(IIII)V");
    CHECK(onProgressMethod, ("Not existing method: void onDownloadProgress(int, int, int, int)"));

    FileToDownload & curFile = g_filesToDownload.back();

    jint curTotal = req.Progress().second;
    jint curProgress = req.Progress().first;
    jint glbTotal = g_totalBytesToDownload;
    jint glbProgress = g_totalDownloadedBytes + req.Progress().first;

    env->CallVoidMethod(*obj.get(), onProgressMethod,
                         curTotal, curProgress,
                         glbTotal, glbProgress);
  }

  void DownloadURLListFinished(downloader::HttpRequest & req,
      downloader::HttpRequest::CallbackT onFinish,
      downloader::HttpRequest::CallbackT onProgress)
  {
    if (req.Status() == downloader::HttpRequest::EFailed)
      onFinish(req);
    else
    {
      FileToDownload & curFile = g_filesToDownload.back();

      LOG(LINFO, ("finished URL list download for", curFile.m_fileName));

      downloader::ParseServerList(req.Data(), curFile.m_urls);

      for (size_t i = 0; i < curFile.m_urls.size(); ++i)
      {
        curFile.m_urls[i] = curFile.m_urls[i] + OMIM_OS_NAME "/"
                          + strings::to_string(g_framework->Storage().GetCurrentVersion()) + "/"
                          + UrlEncode(curFile.m_fileName);
        LOG(LINFO, (curFile.m_urls[i]));
      }

      downloader::HttpRequest::GetFile(curFile.m_urls,
          curFile.m_pathOnSdcard,
          curFile.m_fileSize,
          onFinish,
          onProgress,
          64 * 1024);
    }
  }

  JNIEXPORT int JNICALL
  Java_com_mapswithme_maps_DownloadResourcesActivity_nativeDownloadNextFile(JNIEnv * env,
      jobject thiz, jobject observer)
  {
    if (g_filesToDownload.empty())
      return ERR_NO_MORE_FILES;

    FileToDownload & curFile = g_filesToDownload.back();

    LOG(LINFO, ("downloading", curFile.m_fileName, "sized", curFile.m_fileSize, "bytes"));

    downloader::HttpRequest::CallbackT onFinish(bind(&DownloadFileFinished, jni::make_global_ref(observer), _1));
    downloader::HttpRequest::CallbackT onProgress(bind(&DownloadFileProgress, jni::make_global_ref(observer), _1));

    downloader::HttpRequest::PostJson(GetPlatform().MetaServerUrl(),
                                      curFile.m_fileName,
                                      bind(&DownloadURLListFinished, _1,
                                           onFinish,
                                           onProgress));

    return ERR_FILE_IN_PROGRESS;
  }

  // Move downloaded maps from /sdcard/Android/data/com.mapswithme.maps/files/
  // to /sdcard/MapsWithMe
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_DownloadResourcesActivity_nativeMoveMaps(JNIEnv * env, jobject thiz,
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
