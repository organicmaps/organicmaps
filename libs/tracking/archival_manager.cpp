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
std::string const kTracksArchive = "tracks_archive";
std::string const kFileTimestampName = "latest_upload";

std::chrono::seconds GetTimestamp()
{
  auto const now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
}

std::string GetTimestampFile(std::string const & tracksDir)
{
  return base::JoinPath(tracksDir, kFileTimestampName);
}
}  // namespace

namespace tracking
{
std::string GetTracksDirectory()
{
  return base::JoinPath(GetPlatform().WritableDir(), kTracksArchive);
}

ArchivalManager::ArchivalManager(std::string const & url)
  : m_url(url)
  , m_tracksDir(GetTracksDirectory())
  , m_timestampFile(GetTimestampFile(m_tracksDir))
{}

void ArchivalManager::SetSettings(ArchivingSettings const & settings)
{
  m_settings = settings;
}

std::optional<FileWriter> ArchivalManager::GetFileWriter(routing::RouterType const & trackType) const
{
  std::string const fileName = archival_file::GetArchiveFilename(m_settings.m_version, GetTimestamp(), trackType);
  try
  {
    return std::optional<FileWriter>(base::JoinPath(m_tracksDir, fileName));
  }
  catch (std::exception const & e)
  {
    return std::nullopt;
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

size_t ArchivalManager::IntervalBetweenDumpsSeconds()
{
  return m_settings.m_dumpIntervalSeconds;
}

std::vector<std::string> ArchivalManager::GetFilesOrderedByCreation(std::string const & extension) const
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
  return GetTimeFromLastUploadSeconds() > m_settings.m_uploadIntervalSeconds;
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
  payload.m_headers = {};
  platform::HttpUploaderBackground uploader(payload);
  uploader.Upload();
}

bool ArchivalManager::CanDumpToDisk(size_t neededFreeSpace) const
{
  size_t const neededSize = std::max(m_settings.m_minFreeSpaceOnDiskBytes, neededFreeSpace);
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

size_t ArchivalManager::GetMaxSavedFilesCount(std::string const & extension) const
{
  if (extension == ARCHIVE_TRACKS_FILE_EXTENSION)
    return m_settings.m_maxFilesToSave;
  if (extension == ARCHIVE_TRACKS_ZIPPED_FILE_EXTENSION)
    return m_settings.m_maxArchivesToSave;
  UNREACHABLE();
}

void ArchivalManager::DeleteOldDataByExtension(std::string const & extension) const
{
  auto const files = GetFilesOrderedByCreation(extension);
  auto const maxCount = GetMaxSavedFilesCount(extension);
  if (files.size() > maxCount)
    for (size_t i = 0; i < files.size() - maxCount; ++i)
      base::DeleteFileX(files[i]);
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
