#pragma once

#include "storage/diff_scheme/diffs_data_source.hpp"
#include "storage/storage_defines.hpp"

#include "platform/country_defines.hpp"
#include "platform/country_file.hpp"
#include "platform/downloader_defines.hpp"

#include <string>

namespace storage
{

class QueuedCountry
{
public:
  class Subscriber
  {
  public:
    virtual void OnCountryInQueue(QueuedCountry const & queuedCountry) = 0;
    virtual void OnStartDownloading(QueuedCountry const & queuedCountry) = 0;
    virtual void OnDownloadProgress(QueuedCountry const & queuedCountry, downloader::Progress const & progress) = 0;
    virtual void OnDownloadFinished(QueuedCountry const & queuedCountry, downloader::DownloadStatus status) = 0;

  protected:
    virtual ~Subscriber() = default;
  };

  QueuedCountry(platform::CountryFile const & countryFile, CountryId const & m_countryId, MapFileType type,
                int64_t currentDataVersion, std::string const & dataDir, diffs::DiffsSourcePtr const & diffs);

  void Subscribe(Subscriber & subscriber);
  void Unsubscribe();

  void SetFileType(MapFileType type);
  MapFileType GetFileType() const;

  CountryId const & GetCountryId() const;

  std::string GetRelativeUrl() const;
  std::string GetFileDownloadPath() const;
  uint64_t GetDownloadSize() const;

  void OnCountryInQueue() const;
  void OnStartDownloading() const;
  void OnDownloadProgress(downloader::Progress const & progress) const;
  void OnDownloadFinished(downloader::DownloadStatus status) const;

  bool operator==(CountryId const & countryId) const;

private:
  platform::CountryFile const m_countryFile;
  CountryId const m_countryId;
  MapFileType m_fileType;
  int64_t m_currentDataVersion;
  std::string m_dataDir;
  diffs::DiffsSourcePtr m_diffsDataSource;

  Subscriber * m_subscriber = nullptr;
};
}  // namespace storage
