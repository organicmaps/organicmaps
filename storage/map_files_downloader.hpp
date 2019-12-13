#pragma once

#include "storage/downloading_policy.hpp"
#include "storage/queued_country.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/http_request.hpp"
#include "platform/safe_callback.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace storage
{
using Queue = std::list<QueuedCountry>;
// This interface encapsulates HTTP routines for receiving servers
// URLs and downloading a single map file.
class MapFilesDownloader
{
public:
  // Denotes bytes downloaded and total number of bytes.
  using ServersList = std::vector<std::string>;

  using ServersListCallback = platform::SafeCallback<void(ServersList const & serverList)>;

  class Subscriber
  {
  public:
    virtual ~Subscriber() = default;

    virtual void OnStartDownloading() = 0;
    virtual void OnFinishDownloading() = 0;
  };

  virtual ~MapFilesDownloader() = default;

  /// Asynchronously downloads a map file, periodically invokes
  /// onProgress callback and finally invokes onDownloaded
  /// callback. Both callbacks will be invoked on the main thread.
  void DownloadMapFile(QueuedCountry & queuedCountry);

  /// Returns current downloading progress.
  virtual downloader::Progress GetDownloadingProgress() = 0;

  /// Returns true when downloader does not perform any job.
  virtual bool IsIdle() = 0;

  virtual void Pause() = 0;
  virtual void Resume() = 0;
  virtual void Remove(CountryId const & id) = 0;
  virtual void Clear() = 0;

  virtual Queue const & GetQueue() const = 0;

  void Subscribe(Subscriber * subscriber);
  void UnsubscribeAll();

  static std::string MakeFullUrlLegacy(std::string const & baseUrl, std::string const & fileName,
                                       int64_t dataVersion);

  void SetServersList(ServersList const & serversList);
  void SetDownloadingPolicy(DownloadingPolicy * policy);

protected:
  bool IsDownloadingAllowed() const;
  std::vector<std::string> MakeUrlList(std::string const & relativeUrl);

  // Synchronously loads list of servers by http client.
  static ServersList LoadServersList();

  std::vector<Subscriber *> m_subscribers;

private:
  /// NOTE: this method will be called on network thread.
  /// Default implementation receives a list of all servers that can be asked
  /// for a map file and invokes callback on the main thread.
  virtual void GetServersList(ServersListCallback const & callback);
  /// Asynchronously downloads the file and saves result to provided directory.
  virtual void Download(QueuedCountry & queuedCountry) = 0;

  ServersList m_serversList;
  bool m_isServersListRequested = false;
  DownloadingPolicy * m_downloadingPolicy = nullptr;
};
}  // namespace storage
