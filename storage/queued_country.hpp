#pragma once

#include "storage/diff_scheme/diffs_data_source.hpp"
#include "storage/storage_defines.hpp"

#include "platform/country_defines.hpp"
#include "platform/country_file.hpp"
#include "platform/downloader_defines.hpp"

#include <string>

namespace storage
{
class QueuedCountry;

using DownloadingFinishCallback =
    std::function<void(QueuedCountry const & queuedCountry, downloader::DownloadStatus status)>;
using DownloadingProgressCallback =
    std::function<void(QueuedCountry const & queuedCountry, downloader::Progress const & progress)>;

class QueuedCountry
{
public:
  QueuedCountry(platform::CountryFile const & countryFile, CountryId const & m_countryId,
                MapFileType type, int64_t currentDataVersion, std::string const & dataDir,
                diffs::DiffsSourcePtr const & diffs);

  void SetFileType(MapFileType type);
  MapFileType GetFileType() const;

  CountryId const & GetCountryId() const;

  std::string GetRelativeUrl() const;
  std::string GetFileDownloadPath() const;
  uint64_t GetDownloadSize() const;

  void ClarifyDownloadingType();

  void SetOnFinishCallback(DownloadingFinishCallback const & onDownloaded);
  void SetOnProgressCallback(DownloadingProgressCallback const & onProgress);

  void OnDownloadFinished(downloader::DownloadStatus status) const;
  void OnDownloadProgress(downloader::Progress const & progress) const;

  bool operator==(CountryId const & countryId) const;

private:
  platform::CountryFile const m_countryFile;
  CountryId const m_countryId;
  MapFileType m_fileType;
  int64_t m_currentDataVersion;
  std::string m_dataDir;
  diffs::DiffsSourcePtr m_diffsDataSource;

  DownloadingFinishCallback m_downloadingFinishCallback;
  DownloadingProgressCallback m_downloadingProgressCallback;
};
}  // namespace storage
