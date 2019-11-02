#include "tracking/archival_manager.hpp"

#include "tracking/archival_file.hpp"

#include "platform/http_payload.hpp"
#include "platform/http_uploader_background.hpp"
#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <algorithm>

namespace
{
size_t constexpr KMinFreeSpaceOnDiskBytes = 30 * 1024 * 1024;  // 30 Mb

#ifdef DEBUG
size_t constexpr kDumpIntervalSeconds = 60;
size_t constexpr kMaxFilesToSave = 100;
size_t constexpr kMaxArchivesToSave = 10;
size_t constexpr kUploadIntervalSeconds = 15 * 60;
#else
size_t constexpr kDumpIntervalSeconds = 10 * 60;  // One time per 10 minutes
size_t constexpr kMaxFilesToSave = 1000;
size_t constexpr kMaxArchivesToSave = 10;
size_t constexpr kUploadIntervalSeconds = 12 * 60 * 60;  // One time per 12 hours
#endif

std::string const kTracksArchive = "tracks_archive";
std::string const kFileTimestampName = "latest_upload";

std::chrono::seconds GetTimestamp()
{
  auto const now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
}

std::string GetTracksDirectory()
{
  return base::JoinPath(GetPlatform().WritableDir(), kTracksArchive);
}

std::string GetTimestampFile(std::string const & tracksDir)
{
  return base::JoinPath(tracksDir, kFileTimestampName);
}
}  // namespace

namespace tracking
{
ArchivalManager::ArchivalManager(uint32_t version, std::string const & url)
  : m_url(url)
  , m_version(version)
  , m_tracksDir(GetTracksDirectory())
  , m_timestampFile(GetTimestampFile(m_tracksDir))
{
}

boost::optional<FileWriter> ArchivalManager::GetFileWriter(
    routing::RouterType const & trackType) const
{
  std::string const fileName =
      archival_file::GetArchiveFilename(m_version, GetTimestamp(), trackType);
  try
  {
    return boost::optional<FileWriter>(base::JoinPath(m_tracksDir, fileName));
  }
  catch (std::exception const & e)
  {
    return boost::none;
  }
}

bool ArchivalManager::CreateTracksDir() const
{
  if (!Platform::MkDirChecked(m_tracksDir))
  {
    LOG(LWARNING, ("Directory could not be created", m_tracksDir));
    return false;
  }
  return true;
}

// static
size_t ArchivalManager::IntervalBetweenDumpsSeconds() { return kDumpIntervalSeconds; }

std::vector<std::string> ArchivalManager::GetFilesOrderedByCreation(
    std::string const & extension) const
{
  Platform::FilesList files;
  Platform::GetFilesByExt(m_tracksDir, extension, files);

  for (auto & file : files)
    file = base::JoinPath(m_tracksDir, file);

  // Protocol version and timestamp are contained in the filenames so they are ordered from
  // oldest to newest.
  std::sort(files.begin(), files.end());
  return files;
}

size_t ArchivalManager::GetTimeFromLastUploadSeconds()
{
  auto const timeStamp = GetTimestamp();
  auto const timeStampPrev = ReadTimestamp(m_timestampFile);
  return static_cast<size_t>((timeStamp - timeStampPrev).count());
}

bool ArchivalManager::ReadyToUpload()
{
  return GetTimeFromLastUploadSeconds() > kUploadIntervalSeconds;
}

void ArchivalManager::PrepareUpload(std::vector<std::string> const & files)
{
  if (files.empty())
    return;

  archival_file::FilesAccumulator accum;

  for (auto const & file : files)
    accum.HandleFile(file);

  auto const archives = accum.PrepareArchives(m_tracksDir);
  for (auto const & archive : archives)
    CreateUploadTask(archive);

  WriteTimestamp(m_timestampFile);

  accum.DeleteProcessedFiles();
}

void ArchivalManager::CreateUploadTask(std::string const & filePath)
{
  platform::HttpPayload payload;
  payload.m_url = m_url;
  payload.m_filePath = filePath;
  payload.m_headers = {{"TrackInfo", base::GetNameFromFullPathWithoutExt(filePath)},
                       {"Aloha", GetPlatform().UniqueIdHash()},
                       {"User-Agent", GetPlatform().GetAppUserAgent()}};
  platform::HttpUploaderBackground uploader(payload);
  uploader.Upload();
}

bool ArchivalManager::CanDumpToDisk(size_t neededFreeSpace) const
{
  size_t const neededSize = std::max(KMinFreeSpaceOnDiskBytes, neededFreeSpace);
  auto const storageStatus = GetPlatform().GetWritableStorageStatus(neededSize);
  if (storageStatus != Platform::TStorageStatus::STORAGE_OK)
  {
    LOG(LWARNING, ("Can not dump stats to disk. Storage status:", storageStatus));
    return false;
  }
  return CreateTracksDir();
}

std::chrono::seconds ArchivalManager::ReadTimestamp(std::string const & filePath)
{
  if (!Platform::IsFileExistsByFullPath(filePath))
    return std::chrono::seconds(0);

  try
  {
    FileReader reader(filePath);
    ReaderSource<FileReader> src(reader);
    uint64_t ts = 0;
    ReadPrimitiveFromSource(src, ts);
    return std::chrono::seconds(ts);
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Error reading timestamp from file", e.what()));
  }
  return std::chrono::seconds(0);
}

// static
size_t ArchivalManager::GetMaxSavedFilesCount(std::string const & extension)
{
  if (extension == ARCHIVE_TRACKS_FILE_EXTENSION)
    return kMaxFilesToSave;
  if (extension == ARCHIVE_TRACKS_ZIPPED_FILE_EXTENSION)
    return kMaxArchivesToSave;
  UNREACHABLE();
}

void ArchivalManager::DeleteOldDataByExtension(std::string const & extension) const
{
  auto const files = GetFilesOrderedByCreation(extension);
  auto const maxCount = GetMaxSavedFilesCount(extension);
  if (files.size() > maxCount)
  {
    for (size_t i = 0; i < files.size() - maxCount; ++i)
      base::DeleteFileX(files[i]);
  }
}

void ArchivalManager::WriteTimestamp(std::string const & filePath)
{
  try
  {
    FileWriter writer(filePath);
    uint64_t const ts = GetTimestamp().count();
    WriteToSink(writer, ts);
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Error writing timestamp to file", e.what()));
  }
}

void ArchivalManager::PrepareUpload()
{
  if (!ReadyToUpload())
    return;

  auto files = GetFilesOrderedByCreation(ARCHIVE_TRACKS_FILE_EXTENSION);
  PrepareUpload(files);
}
}  // namespace tracking
