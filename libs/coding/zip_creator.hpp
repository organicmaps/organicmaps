#pragma once

#include <string>
#include <vector>
#include "3party/minizip/minizip.hpp"

enum class CompressionLevel
{
  NoCompression = 0,
  BestSpeed,
  BestCompression,
  DefaultCompression,
  Count
};

void FillZipLocalDateTime(zip::DateTime & res);

bool CreateZipFromFiles(std::vector<std::string> const & files, std::vector<std::string> const & filesInArchive,
                        std::string const & zipFilePath,
                        CompressionLevel compression = CompressionLevel::DefaultCompression);

bool CreateZipFromFiles(std::vector<std::string> const & files, std::string const & zipFilePath,
                        CompressionLevel compression = CompressionLevel::DefaultCompression);
