#include "com/mapswithme/maps/DownloaderAdapter.hpp"

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/platform/Platform.hpp"

#include "storage/downloader.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"

#include <memory>
#include <unordered_map>
#include <utility>

using Callbacks = std::pair<std::function<void(bool status)>,
                            std::function<void(int64_t downloaded, int64_t total)>>;
static std::unordered_map<int64_t, Callbacks> g_completionHandlers;

namespace storage
{
BackgroundDownloaderAdapter::BackgroundDownloaderAdapter()
{
  auto env = jni::GetEnv();
  auto downloadManagerClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/downloader/MapDownloadManager");
  auto from = jni::GetStaticMethodID(env, downloadManagerClazz, "from",
                           "(Landroid/content/Context;)Lcom/mapswithme/maps/downloader/MapDownloadManager;");

  auto context = android::Platform::Instance().GetContext();
  m_downloadManager = jni::make_global_ref(env->CallStaticObjectMethod(downloadManagerClazz, from,  context));
  m_downloadManagerRemove = env->GetMethodID(downloadManagerClazz, "remove","(J)V");
  m_downloadManagerEnqueue = env->GetMethodID(downloadManagerClazz, "enqueue", "(Ljava/lang/String;)J");
  jni::HandleJavaException(env);
}

BackgroundDownloaderAdapter::~BackgroundDownloaderAdapter()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  g_completionHandlers.clear();
}

void BackgroundDownloaderAdapter::Remove(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  MapFilesDownloader::Remove(countryId);

  if (!m_queue.Contains(countryId))
    return;

  auto const id = m_queue.GetTaskInfoForCountryId(countryId);
  if (id)
  {
    RemoveByRequestId(*id);
    g_completionHandlers.erase(*id);
  }
  m_queue.Remove(countryId);
}

void BackgroundDownloaderAdapter::Clear()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  MapFilesDownloader::Clear();

  m_queue.ForEachTaskInfo([this](auto const id)
  {
    RemoveByRequestId(id);
    g_completionHandlers.erase(id);
  });

  m_queue.Clear();
}

QueueInterface const & BackgroundDownloaderAdapter::GetQueue() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (m_queue.IsEmpty())
    return MapFilesDownloader::GetQueue();

  return m_queue;
}

void BackgroundDownloaderAdapter::Download(QueuedCountry && queuedCountry)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!IsDownloadingAllowed())
  {
    queuedCountry.OnDownloadFinished(downloader::DownloadStatus::Failed);
    return;
  }

  auto const countryId = queuedCountry.GetCountryId();
  auto urls = MakeUrlList(queuedCountry.GetRelativeUrl());
  auto const path = queuedCountry.GetFileDownloadPath();

  queuedCountry.OnStartDownloading();

  m_queue.Append(std::move(queuedCountry));

  DownloadFromLastUrl(countryId, path, std::move(urls));
}

void BackgroundDownloaderAdapter::DownloadFromLastUrl(CountryId const & countryId,
                                                     std::string const & downloadPath,
                                                     std::vector<std::string> && urls)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (urls.empty())
    return;

  auto env = jni::GetEnv();

  jni::TScopedLocalRef url(env, jni::ToJavaString(env, urls.back()));
  auto id = static_cast<int64_t>(env->CallLongMethod(*m_downloadManager, m_downloadManagerEnqueue, url.get()));
  urls.pop_back();

  jni::HandleJavaException(env);

  m_queue.SetTaskInfoForCountryId(countryId, id);

  auto onFinish = [this, countryId, downloadPath, urls = std::move(urls)](bool status) mutable
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());

    if (!m_queue.Contains(countryId))
      return;

    if (!status && !urls.empty())
    {
      DownloadFromLastUrl(countryId, downloadPath, std::move(urls));
    }
    else
    {
      auto const country = m_queue.GetCountryById(countryId);
      m_queue.Remove(countryId);
      country.OnDownloadFinished(status
                                 ? downloader::DownloadStatus::Completed
                                 : downloader::DownloadStatus::Failed);
    }
  };

  auto onProgress = [this, countryId](int64_t bytesDownloaded, int64_t bytesTotal)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());

    if (!m_queue.Contains(countryId))
      return;

    auto const & country = m_queue.GetCountryById(countryId);
    country.OnDownloadProgress({bytesDownloaded, bytesTotal});
  };

  g_completionHandlers.emplace(id, Callbacks(onFinish, onProgress));
}

void BackgroundDownloaderAdapter::RemoveByRequestId(int64_t id)
{
  auto env = jni::GetEnv();
  env->CallVoidMethod(*m_downloadManager, m_downloadManagerRemove, static_cast<jlong>(id));

  jni::HandleJavaException(env);
}

std::unique_ptr<MapFilesDownloader> GetDownloader()
{
  return std::make_unique<BackgroundDownloaderAdapter>();
}
}  // namespace storage

extern "C" {
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeOnDownloadFinished(JNIEnv *, jclass,
                                                                        jboolean status, jlong id)
{
  auto const it = g_completionHandlers.find(static_cast<int64_t>(id));
  if (it == g_completionHandlers.end())
    return;

  it->second.first(static_cast<bool>(status));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_downloader_MapManager_nativeOnDownloadProgress(JNIEnv *, jclass, jlong id,
                                                                        jlong bytesDownloaded,
                                                                        jlong bytesTotal)
{
  auto const it = g_completionHandlers.find(static_cast<int64_t>(id));
  if (it == g_completionHandlers.end())
    return;

  it->second.second(static_cast<int64_t>(bytesDownloaded), static_cast<int64_t>(bytesTotal));
}
}  // extern "C"
