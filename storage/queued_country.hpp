#pragma once

#include "storage/diff_scheme/diffs_data_source.hpp"
#include "storage/storage_defines.hpp"

#include "platform/country_defines.hpp"
#include "platform/country_file.hpp"

#include <string>

namespace storage
{
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

  bool operator==(CountryId const & countryId) const;

private:
  platform::CountryFile const m_countryFile;
  CountryId const m_countryId;
  MapFileType m_fileType;
  int64_t m_currentDataVersion;
  std::string m_dataDir;
  std::shared_ptr<diffs::DiffsDataSource> m_diffsDataSource;
};
}  // namespace storage
