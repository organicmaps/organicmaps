#pragma once

#include "storage/downloader_queue_universal.hpp"
#include "storage/downloading_policy.hpp"
#include "storage/queued_country.hpp"

#include "platform/http_client.hpp"
#include "platform/servers_list.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace storage
{

// This interface encapsulates HTTP routines for receiving servers
// URLs and downloading a single map file.
class MapFilesDownloader
{
public:
  // Denotes bytes downloaded and total number of bytes.
  using ServersList = std::vector<std::string>;
  using MetaConfigCallback = std::function<void(downloader::MetaConfig && metaConfig)>;

  virtual ~MapFilesDownloader();

  /// Asynchronously downloads a map file, periodically invokes
  /// onProgress callback and finally invokes onDownloaded
  /// callback. Both callbacks will be invoked on the main thread.
  void DownloadMapFile(QueuedCountry && queuedCountry);

  // Removes item from m_quarantine queue when list of servers is not received.
  // Parent method must be called into override method.
  virtual void Remove(CountryId const & id);

  // Clears m_quarantine queue when list of servers is not received.
  // Parent method must be called into override method.
  virtual void Clear();

  // Returns m_quarantine queue when list of servers is not received.
  // Parent method must be called into override method.
  virtual QueueInterface const & GetQueue() const;

  /**
   * @brief Async file download as string buffer (for small files only).
   * Request can be skipped if current servers list is empty.
   * Callback will be skipped on download error.
   * @param[in]  url        Final url part like "index.json" or "maps/210415/countries.txt".
   * @param[in]  forceReset True - force reset current request, if any.
   */
  void DownloadAsString(std::string url, std::function<bool(std::string const &)> && callback, bool forceReset = false);

  void SetServersList(ServersList const & serversList);
  void SetDownloadingPolicy(DownloadingPolicy * policy);
  void SetDataVersion(int64_t version) { m_dataVersion = version; }

  /// @name Legacy functions for Android resourses downloading routine.
  /// @{
  void EnsureMetaConfigReady(std::function<void()> && callback);
  std::vector<std::string> MakeUrlListLegacy(std::string const & fileName) const;
  /// @}

protected:
  bool IsDownloadingAllowed() const;
  std::vector<std::string> MakeUrlList(std::string const & relativeUrl) const;

  // Synchronously loads list of servers by http client.
  downloader::MetaConfig LoadMetaConfig() const;

private:
  /**
   * @brief This method is blocking and should be called on network thread.
   * Default implementation receives a list of all servers that can be asked
   * for a map file and invokes callback on the main thread.
   */
  virtual downloader::MetaConfig GetMetaConfig();
  /// Asynchronously downloads the file and saves result to provided directory.
  virtual void Download(QueuedCountry && queuedCountry) = 0;

  /// Starts the meta-config network fetch. On completion, drains all m_metaConfigWaiters on the GUI thread.
  void RunMetaConfigAsync();

  /// Current file downloading handle for DownloadAsString.
  std::optional<platform::HttpClient::RequestHandle> m_downloadHandle;

  ServersList m_serversList;
  int64_t m_dataVersion = 0;

  /// Used as guard for m_serversList assign.
  std::atomic_bool m_isMetaConfigRequested = false;

  DownloadingPolicy * m_downloadingPolicy = nullptr;

  // This queue accumulates download requests before
  // the servers list is received on the network thread.
  Queue m_pendingRequests;

  // Shared alive flag for safe async lambda capture. Set to false in destructor.
  // Lambdas capture this by shared_ptr copy, so it remains valid after destruction.
  std::shared_ptr<std::atomic<bool>> m_alive = std::make_shared<std::atomic<bool>>(true);

  // Incremented before starting new DownloadAsString requests (GUI-thread only).
  // Captured by GUI-thread lambdas to detect superseded requests.
  uint64_t m_generation = 0;

  // Continuations waiting for meta-config to become ready (GUI-thread only).
  // Drained by RunMetaConfigAsync after m_serversList is assigned.
  std::vector<std::function<void()>> m_metaConfigWaiters;
};
}  // namespace storage
