#pragma once

#include "routing/router.hpp"

#include <chrono>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace tracking
{
namespace archival_file
{
/// \brief Data contained in the tracks filename
struct FileInfo
{
  uint64_t m_timestamp = 0;
  routing::RouterType m_trackType = routing::RouterType::Vehicle;
  uint32_t m_protocolVersion = std::numeric_limits<uint8_t>::max();
};

/// \brief Tracks files with total size for archiving in single zip
struct FilesBatch
{
  size_t m_totalSize;
  std::vector<std::string> m_files;
};

/// \brief Helper-class for zipping tracks and deleting respective source files
class FilesAccumulator
{
public:
  void HandleFile(std::string const & fileName);
  std::vector<std::string> PrepareArchives(std::string const & path);
  void DeleteProcessedFiles();

private:
  std::map<routing::RouterType, FilesBatch> m_filesByType;
};

/// \returns file name with extension containing |protocolVersion|, |timestamp| and |trackType|
/// of the archived tracks, separated by the delimiter. Example: 1_1573635326_0.track
std::string GetArchiveFilename(uint8_t protocolVersion, std::chrono::seconds timestamp,
                               routing::RouterType const & trackType);

/// \returns meta-information about the archived tracks saved in the |fileName|.
FileInfo ParseArchiveFilename(std::string const & fileName);
}  // namespace archival_file
}  // namespace tracking
