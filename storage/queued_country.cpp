#include "storage/queued_country.hpp"

#include "storage/storage_helpers.hpp"

#include "platform/local_country_file_utils.hpp"

#include "coding/url_encode.hpp"

#include "base/assert.hpp"
#include "base/url_helpers.hpp"

namespace
{
std::string MakeRelativeUrl(std::string const & fileName, int64_t dataVersion, uint64_t diffVersion)
{
  std::ostringstream url;
  if (diffVersion != 0)
    url << "diffs/" << dataVersion << "/" << diffVersion;
  else
    url << OMIM_OS_NAME "/" << dataVersion;

  return base::url::Join(url.str(), UrlEncode(fileName));
}
}  // namespace

namespace storage
{
QueuedCountry::QueuedCountry(platform::CountryFile const & countryFile, CountryId const & countryId,
                             MapFileType type, int64_t currentDataVersion,
                             std::string const & dataDir,
                             diffs::DiffsSourcePtr const & diffs)
  : m_countryFile(countryFile)
  , m_countryId(countryId)
  , m_fileType(type)
  , m_currentDataVersion(currentDataVersion)
  , m_dataDir(dataDir)
  , m_diffsDataSource(diffs)
{
  ASSERT(IsCountryIdValid(GetCountryId()), ("Only valid countries may be downloaded."));
  ASSERT(m_diffsDataSource != nullptr, ());
}

void QueuedCountry::SetFileType(MapFileType type)
{
  m_fileType = type;
}

MapFileType QueuedCountry::GetFileType() const
{
  return m_fileType;
}

CountryId const & QueuedCountry::GetCountryId() const
{
  return m_countryId;
}

std::string QueuedCountry::GetRelativeUrl() const
{
  auto const fileName = platform::GetFileName(m_countryFile.GetName(), m_fileType);

  uint64_t diffVersion = 0;
  if (m_fileType == MapFileType::Diff)
    CHECK(m_diffsDataSource->VersionFor(m_countryId, diffVersion), ());

  return MakeRelativeUrl(fileName, m_currentDataVersion, diffVersion);
}

std::string QueuedCountry::GetFileDownloadPath() const
{
  return platform::GetFileDownloadPath(m_currentDataVersion, m_dataDir, m_countryFile, m_fileType);
}

uint64_t QueuedCountry::GetDownloadSize() const
{
  uint64_t size;
  if (m_fileType == MapFileType::Diff)
  {
    CHECK(m_diffsDataSource->SizeToDownloadFor(m_countryId, size), ());
    return size;
  }

  return GetRemoteSize(*m_diffsDataSource, m_countryFile, m_currentDataVersion);
}

void QueuedCountry::ClarifyDownloadingType()
{
  if (m_fileType != MapFileType::Diff)
    return;

  using diffs::Status;
  auto const status = m_diffsDataSource->GetStatus();
  if (status == Status::NotAvailable ||
     (status == Status::Available && !m_diffsDataSource->HasDiffFor(m_countryId)))
  {
    m_fileType = MapFileType::Map;
  }
}

void QueuedCountry::SetOnFinishCallback(DownloadingFinishCallback const & onDownloaded)
{
  m_downloadingFinishCallback = onDownloaded;
}

void QueuedCountry::SetOnProgressCallback(DownloadingProgressCallback const & onProgress)
{
  m_downloadingProgressCallback = onProgress;
}

void QueuedCountry::OnDownloadFinished(downloader::DownloadStatus status) const
{
  if (m_downloadingFinishCallback)
    m_downloadingFinishCallback(*this, status);
}

void QueuedCountry::OnDownloadProgress(downloader::Progress const & progress) const
{
  if (m_downloadingProgressCallback)
    m_downloadingProgressCallback(*this, progress);
}

bool QueuedCountry::operator==(CountryId const & countryId) const
{
  return m_countryId == countryId;
}
}  // namespace storage
