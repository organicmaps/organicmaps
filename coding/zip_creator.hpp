#pragma once

#include <string>
#include <vector>

enum class CompressionLevel
{
  NoCompression = 0,
  BestSpeed,
  BestCompression,
  DefaultCompression,
  Count
};

/// @param[in]  filePaths Full paths on disk to archive.
/// @param[in]  fileNames Correspondent (for filePaths) file names in archive.
bool CreateZipFromFiles(std::vector<std::string> const & filePaths, std::string const & zipFilePath,
                        CompressionLevel compression = CompressionLevel::DefaultCompression,
                        std::vector<std::string> const * fileNames = nullptr);
