#include "tracking/archival_file.hpp"

#include "platform/platform.hpp"

#include "coding/zip_creator.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <exception>

#include "defines.hpp"

namespace
{
#ifdef DEBUG
size_t constexpr kMaxFilesTotalSizeToSendBytes = 100 * 1024;  // 0.1 Mb
#else
size_t constexpr kMaxFilesTotalSizeToSendBytes = 1000 * 1024;  // 1 Mb
#endif

char constexpr kDelimiter = '_';
}  // namespace

namespace tracking
{
namespace archival_file
{
void FilesAccumulator::HandleFile(std::string const & fileName)
{
  uint64_t fileSize = 0;
  if (!Platform::GetFileSizeByFullPath(fileName, fileSize))
  {
    LOG(LDEBUG, ("File does not exist", fileName));
    return;
  }
  if (fileSize == 0)
  {
    LOG(LDEBUG, ("File is empty", fileName));
    base::DeleteFileX(fileName);
    return;
  }
  if (fileSize > kMaxFilesTotalSizeToSendBytes)
  {
    LOG(LDEBUG, ("File is too large", fileName, fileSize));
    base::DeleteFileX(fileName);
    return;
  }

  FileInfo const meta = ParseArchiveFilename(fileName);
  auto const insData = m_filesByType.emplace(meta.m_trackType, FilesBatch());
  auto const it = insData.first;
  auto & fileBatch = it->second;
  if (!insData.second)
  {
    if (fileBatch.m_totalSize + fileSize > kMaxFilesTotalSizeToSendBytes)
      return;
  }

  fileBatch.m_totalSize += fileSize;
  fileBatch.m_files.push_back(fileName);
}

std::vector<std::string> FilesAccumulator::PrepareArchives(std::string const & path)
{
  std::vector<std::string> archives;
  try
  {
    for (auto const & it : m_filesByType)
    {
      if (it.second.m_files.empty())
        continue;
      std::string const archivePath = base::JoinPath(
          path, base::GetNameFromFullPathWithoutExt(it.second.m_files[0]) + ARCHIVE_TRACKS_ZIPPED_FILE_EXTENSION);

      if (CreateZipFromFiles(it.second.m_files, archivePath, CompressionLevel::NoCompression))
        archives.emplace_back(archivePath);
    }
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Error while archiving files", e.what()));
  }
  return archives;
}

void FilesAccumulator::DeleteProcessedFiles()
{
  for (auto const & it : m_filesByType)
    for (auto const & file : it.second.m_files)
      base::DeleteFileX(file);
}

std::string GetArchiveFilename(uint8_t protocolVersion, std::chrono::seconds timestamp,
                               routing::RouterType const & trackType)
{
  std::string filename;
  size_t constexpr kTrackFilenameSize = 20;
  filename.reserve(kTrackFilenameSize);  // All filename parts have fixed length.
  filename = std::to_string(protocolVersion) + kDelimiter + std::to_string(timestamp.count()) + kDelimiter +
             std::to_string(static_cast<uint8_t>(trackType)) + ARCHIVE_TRACKS_FILE_EXTENSION;
  CHECK_EQUAL(filename.size(), kTrackFilenameSize, ());
  return filename;
}

FileInfo ParseArchiveFilename(std::string const & fileName)
{
  std::string const metaData = base::GetNameFromFullPathWithoutExt(fileName);
  size_t const indexFirstDelim = metaData.find(kDelimiter);
  size_t const indexLastDelim = metaData.rfind(kDelimiter);
  if (indexFirstDelim != 1 || indexLastDelim != 12)
  {
    LOG(LWARNING, ("Could not find delimiters in filename", fileName));
    return {};
  }

  try
  {
    FileInfo res;
    res.m_protocolVersion = static_cast<uint32_t>(std::stoul(metaData.substr(0, indexFirstDelim)));
    res.m_timestamp = std::stoul(metaData.substr(indexFirstDelim + 1, indexLastDelim - indexFirstDelim - 1));
    res.m_trackType = static_cast<routing::RouterType>(std::stoul(metaData.substr(indexLastDelim + 1)));
    return res;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Error while parsing filename", e.what()));
  }
  return {};
}
}  // namespace archival_file
}  // namespace tracking
