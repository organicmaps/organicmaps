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

bool CreateZipFromPathDeflatedAndDefaultCompression(std::string const & filePath,
                                                    std::string const & zipFilePath);

bool CreateZipFromFiles(std::vector<std::string> const & files, std::string const & zipFilePath,
                        CompressionLevel compression = CompressionLevel::DefaultCompression);
