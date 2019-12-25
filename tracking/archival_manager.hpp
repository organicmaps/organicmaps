#pragma once

#include "routing/router.hpp"

#include "coding/file_writer.hpp"

#include "base/logging.hpp"

#include <chrono>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tracking
{
class ArchivalManager
{
public:
  ArchivalManager(uint32_t version, std::string const & url);

  ArchivalManager(ArchivalManager const &) = delete;
  ArchivalManager & operator=(ArchivalManager const &) = delete;

  /// \brief Saves to file contents of the |archive| if it is necessary to |dumpAnyway|
  /// or the |archive| is ready to be dumped.
  template <class T>
  void Dump(T & archive, routing::RouterType const & trackType, bool dumpAnyway);

  /// \returns time span between Archive dumps.
  static size_t IntervalBetweenDumpsSeconds();

  /// \brief Prepares zipped files and creates task for uploading them.
  void PrepareUpload();

  /// \brief Deletes the oldest files if the total number of archive files is exceeded.
  void DeleteOldDataByExtension(std::string const & extension) const;

private:
  bool ReadyToUpload();

  size_t GetTimeFromLastUploadSeconds();
  static size_t GetMaxSavedFilesCount(std::string const & extension);
  std::chrono::seconds ReadTimestamp(std::string const & filePath);
  void WriteTimestamp(std::string const & filePath);

  std::optional<FileWriter> GetFileWriter(routing::RouterType const & trackType) const;

  std::vector<std::string> GetFilesOrderedByCreation(std::string const & extension) const;
  bool CreateTracksDir() const;
  bool CanDumpToDisk(size_t neededFreeSpace) const;

  void PrepareUpload(std::vector<std::string> const & files);
  void CreateUploadTask(std::string const & filePath);

  std::string const m_url;
  uint32_t const m_version;

  std::string m_tracksDir;
  std::string m_timestampFile;
};

template <class T>
void ArchivalManager::Dump(T & archive, routing::RouterType const & trackType, bool dumpAnyway)
{
  if (!(archive.ReadyToDump() || (dumpAnyway && archive.Size() > 0)))
    return;

  if (!CanDumpToDisk(archive.Size()))
    return;

  if (auto dst = GetFileWriter(trackType))
    archive.Write(*dst);
}
}  // namespace tracking
