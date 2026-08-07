#include "Framework.hpp"

#include "defines.hpp"

#include "storage/downloader.hpp"
#include "storage/storage.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/http_request.hpp"
#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/reader_streambuf.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

/// Special error codes to notify GUI about free space
//@{
#define ERR_DOWNLOAD_SUCCESS      0
#define ERR_DISK_ERROR            -1
#define ERR_NOT_ENOUGH_FREE_SPACE -2
#define ERR_STORAGE_DISCONNECTED  -3
#define ERR_DOWNLOAD_ERROR        -4
#define ERR_NO_MORE_FILES         -5
#define ERR_FILE_IN_PROGRESS      -6
//@}

namespace
{

static std::vector<platform::CountryFile> g_filesToDownload;
static uint64_t g_totalDownloadedBytes;
static uint64_t g_totalBytesToDownload;
static std::shared_ptr<downloader::HttpRequest> g_currentRequest;

}  // namespace

extern "C"
{
JNIEXPORT jint Java_app_organicmaps_sdk_DownloadResourcesLegacyActivity_nativeGetBytesToDownload(JNIEnv * env,
                                                                                                 jclass clazz)
{
  // clear all
  g_filesToDownload.clear();
  g_totalBytesToDownload = 0;
  g_totalDownloadedBytes = 0;

  auto const & storage = g_framework->GetStorage();
  auto const status = storage.GetForceDownloadWorlds(g_filesToDownload);

  for (auto const & cf : g_filesToDownload)
    g_totalBytesToDownload += cf.GetRemoteSize();

  // Only World and WorldCoasts are downloaded here, so the total always fits into jint.
  jint res;
  using enum storage::Storage::WorldStatus;
  if (status == ERROR_CREATE_FOLDER || status == ERROR_MOVE_FILE)
    res = ERR_DISK_ERROR;
  else
  {
    auto const & pl = GetPlatform();
    switch (pl.GetWritableStorageStatus(g_totalBytesToDownload))
    {
    case Platform::STORAGE_DISCONNECTED:
      res = ERR_STORAGE_DISCONNECTED;
      LOG(LWARNING, ("External file system is not available"));
      break;

    case Platform::NOT_ENOUGH_SPACE:
      res = ERR_NOT_ENOUGH_FREE_SPACE;
      LOG(LWARNING, ("Not enough space to download files of", g_totalBytesToDownload, "bytes"));
      break;

    default: res = static_cast<jint>(g_totalBytesToDownload);
    }
  }

  g_currentRequest.reset();

  if (status == WAS_MOVED)
  {
    g_framework->ReloadWorldMaps();
    res = ERR_DOWNLOAD_SUCCESS;  // reset possible storage error if we moved files
  }

  return res;
}

static void DownloadFileFinished(std::shared_ptr<jobject> obj, downloader::HttpRequest const & req)
{
  using downloader::DownloadStatus;

  auto const status = req.GetStatus();
  ASSERT_NOT_EQUAL(status, DownloadStatus::InProgress, ());

  int errorCode = ERR_DOWNLOAD_ERROR;
  if (status == DownloadStatus::Completed)
    errorCode = ERR_DOWNLOAD_SUCCESS;

  g_currentRequest.reset();

  if (errorCode == ERR_DOWNLOAD_SUCCESS)
  {
    auto const & curFile = g_filesToDownload.back();
    size_t const sz = curFile.GetRemoteSize();
    LOG(LDEBUG, ("finished downloading", curFile.GetName(), "size", sz, "bytes"));

    g_totalDownloadedBytes += sz;
    LOG(LDEBUG, ("totalDownloadedBytes:", g_totalDownloadedBytes));

    g_filesToDownload.pop_back();
  }

  JNIEnv * env = jni::GetEnv();

  jmethodID methodID = jni::GetMethodID(env, *obj, "onFinish", "(I)V");
  env->CallVoidMethod(*obj, methodID, errorCode);
}

static void DownloadFileProgress(std::shared_ptr<jobject> listener, downloader::HttpRequest const & req)
{
  JNIEnv * env = jni::GetEnv();
  static jmethodID methodID = jni::GetMethodID(env, *listener, "onProgress", "(I)V");
  env->CallVoidMethod(*listener, methodID,
                      static_cast<jint>(g_totalDownloadedBytes + req.GetProgress().m_bytesDownloaded));
}

JNIEXPORT jint Java_app_organicmaps_sdk_DownloadResourcesLegacyActivity_nativeStartNextFileDownload(JNIEnv * env,
                                                                                                    jclass clazz,
                                                                                                    jobject listener)
{
  using namespace std::placeholders;

  if (g_filesToDownload.empty())
    return ERR_NO_MORE_FILES;

  /// @todo One downloader instance with cached servers. All this routine will be refactored some time.
  static auto downloader = storage::GetDownloader();
  auto const & storage = g_framework->GetStorage();
  downloader->SetDataVersion(storage.GetCurrentDataVersion());

  downloader->EnsureMetaConfigReady([&storage, ptr = jni::make_global_ref(listener)]()
  {
    auto const & curFile = g_filesToDownload.back();
    auto const fileName = curFile.GetFileName(MapFileType::Map);
    LOG(LINFO, ("Downloading file", fileName));

    g_currentRequest.reset(downloader::HttpRequest::GetFile(
        downloader->MakeUrlListLegacy(fileName), storage.GetFilePath(curFile.GetName(), MapFileType::Map),
        curFile.GetRemoteSize(), std::bind(&DownloadFileFinished, ptr, _1), std::bind(&DownloadFileProgress, ptr, _1),
        false));
  });

  return ERR_FILE_IN_PROGRESS;
}

JNIEXPORT void Java_app_organicmaps_sdk_DownloadResourcesLegacyActivity_nativeCancelCurrentFile(JNIEnv * env,
                                                                                                jclass clazz)
{
  LOG(LDEBUG, ("cancelCurrentFile, currentRequest=", g_currentRequest));
  g_currentRequest.reset();
}
}
