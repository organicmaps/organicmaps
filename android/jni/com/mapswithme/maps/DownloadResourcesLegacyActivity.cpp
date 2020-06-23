#include "Framework.hpp"

#include "defines.hpp"

#include "storage/map_files_downloader.hpp"
#include "storage/storage.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/http_request.hpp"
#include "platform/platform.hpp"
#include "platform/servers_list.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/reader_streambuf.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "com/mapswithme/core/jni_helper.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace downloader;
using namespace storage;

using namespace std::placeholders;

/// Special error codes to notify GUI about free space
//@{
#define ERR_DOWNLOAD_SUCCESS         0
#define ERR_NOT_ENOUGH_MEMORY       -1
#define ERR_NOT_ENOUGH_FREE_SPACE   -2
#define ERR_STORAGE_DISCONNECTED    -3
#define ERR_DOWNLOAD_ERROR          -4
#define ERR_NO_MORE_FILES           -5
#define ERR_FILE_IN_PROGRESS        -6
//@}

struct FileToDownload
{
  std::vector<std::string> m_urls;
  std::string m_fileName;
  std::string m_pathOnSdcard;
  uint64_t m_fileSize;
};

namespace
{

static std::vector<FileToDownload> g_filesToDownload;
static int g_totalDownloadedBytes;
static int g_totalBytesToDownload;
static std::shared_ptr<HttpRequest> g_currentRequest;

}  // namespace

extern "C"
{
  using Callback = HttpRequest::Callback;

  static int HasSpaceForFiles(Platform & pl, std::string const & sdcardPath, size_t fileSize)
  {
    switch (pl.GetWritableStorageStatus(fileSize))
    {
      case Platform::STORAGE_DISCONNECTED:
        return ERR_STORAGE_DISCONNECTED;

      case Platform::NOT_ENOUGH_SPACE:
        return ERR_NOT_ENOUGH_FREE_SPACE;

      default:
        return static_cast<int>(fileSize);
    }
  }

  // Check if we need to download mandatory resource file.
  static bool NeedToDownload(Platform & pl, std::string const & name, int size)
  {
    try
    {
      ModelReaderPtr reader(pl.GetReader(name));
      return false;
    }
    catch (RootException const &)
    {
      // TODO: A hack for a changed font file name. Remove when this stops being relevant.
      // This is because we don't want to force update users with the old font (which is good enough)
      if (name == "02_droidsans-fallback.ttf")
        return NeedToDownload(pl, "02_wqy-microhei.ttf", size); // Size is not checked anyway
    }
    return true;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadResourcesLegacyActivity_nativeGetBytesToDownload(JNIEnv * env, jclass clazz)
  {
    // clear all
    g_filesToDownload.clear();
    g_totalBytesToDownload = 0;
    g_totalDownloadedBytes = 0;

    Platform & pl = GetPlatform();
    std::string const path = pl.WritableDir();

    ReaderStreamBuf buffer(pl.GetReader(EXTERNAL_RESOURCES_FILE));
    std::istream in(&buffer);

    std::string name;
    int size;
    while (true)
    {
      in >> name;
      if (!in.good())
        break;

      in >> size;
      if (!in.good())
        break;

      if (NeedToDownload(pl, name, size))
      {
        LOG(LDEBUG, ("Should download", name, "size", size, "bytes"));

        FileToDownload f;
        f.m_pathOnSdcard = path + name;
        f.m_fileName = name;
        f.m_fileSize = size;

        g_filesToDownload.push_back(f);
        g_totalBytesToDownload += size;
      }
    }

    int const res = HasSpaceForFiles(pl, path, g_totalBytesToDownload);
    if (res == ERR_STORAGE_DISCONNECTED)
      LOG(LWARNING, ("External file system is not available"));
    else if (res == ERR_NOT_ENOUGH_FREE_SPACE)
      LOG(LWARNING, ("Not enough space to extract files"));

    g_currentRequest.reset();

    return res;
  }

  static void DownloadFileFinished(std::shared_ptr<jobject> obj, HttpRequest const & req)
  {
    auto const status = req.GetStatus();
    ASSERT_NOT_EQUAL(status, DownloadStatus::InProgress, ());

    int errorCode = ERR_DOWNLOAD_ERROR;
    if (status == DownloadStatus::Completed)
      errorCode = ERR_DOWNLOAD_SUCCESS;

    g_currentRequest.reset();

    if (errorCode == ERR_DOWNLOAD_SUCCESS)
    {
      FileToDownload & curFile = g_filesToDownload.back();
      LOG(LDEBUG, ("finished downloading", curFile.m_fileName, "size", curFile.m_fileSize, "bytes"));

      g_totalDownloadedBytes += curFile.m_fileSize;
      LOG(LDEBUG, ("totalDownloadedBytes:", g_totalDownloadedBytes));

      g_filesToDownload.pop_back();
    }

    JNIEnv * env = jni::GetEnv();

    jmethodID methodID = jni::GetMethodID(env, *obj, "onFinish", "(I)V");
    env->CallVoidMethod(*obj, methodID, errorCode);
  }

  static void DownloadFileProgress(std::shared_ptr<jobject> listener, HttpRequest const & req)
  {
    JNIEnv * env = jni::GetEnv();
    static jmethodID methodID = jni::GetMethodID(env, *listener, "onProgress", "(I)V");
    env->CallVoidMethod(*listener, methodID, static_cast<jint>(g_totalDownloadedBytes + req.GetProgress().m_bytesDownloaded));
  }

  static void DownloadURLListFinished(HttpRequest const & req, Callback const & onFinish, Callback const & onProgress)
  {
    FileToDownload & curFile = g_filesToDownload.back();

    LOG(LINFO, ("Finished URL list download for", curFile.m_fileName));

    GetServersList(req, curFile.m_urls);

    Storage const & storage = g_framework->GetStorage();
    for (size_t i = 0; i < curFile.m_urls.size(); ++i)
    {
      curFile.m_urls[i] = MapFilesDownloader::MakeFullUrlLegacy(curFile.m_urls[i],
                                                                curFile.m_fileName,
                                                                storage.GetCurrentDataVersion());
      LOG(LDEBUG, (curFile.m_urls[i]));
    }

    g_currentRequest.reset(HttpRequest::GetFile(curFile.m_urls, curFile.m_pathOnSdcard, curFile.m_fileSize, onFinish, onProgress, 512 * 1024, false));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadResourcesLegacyActivity_nativeStartNextFileDownload(JNIEnv * env, jclass clazz, jobject listener)
  {
    if (g_filesToDownload.empty())
      return ERR_NO_MORE_FILES;

    FileToDownload & curFile = g_filesToDownload.back();

    LOG(LDEBUG, ("downloading", curFile.m_fileName, "sized", curFile.m_fileSize, "bytes"));

    Callback onFinish(std::bind(&DownloadFileFinished, jni::make_global_ref(listener), _1));
    Callback onProgress(std::bind(&DownloadFileProgress, jni::make_global_ref(listener), _1));

    g_currentRequest.reset(HttpRequest::PostJson(GetPlatform().ResourcesMetaServerUrl(), curFile.m_fileName,
                                                 std::bind(&DownloadURLListFinished, _1, onFinish, onProgress)));
    return ERR_FILE_IN_PROGRESS;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_DownloadResourcesLegacyActivity_nativeCancelCurrentFile(JNIEnv * env, jclass clazz)
  {
    LOG(LDEBUG, ("cancelCurrentFile, currentRequest=", g_currentRequest));
    g_currentRequest.reset();
  }
}
