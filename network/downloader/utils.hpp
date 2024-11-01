#pragma once

#include <cstdint>
#include <string>

namespace om::network::downloader
{
std::string GetFileDownloadUrl(std::string const & fileName, int64_t dataVersion, uint64_t diffVersion = 0);

bool IsUrlSupported(std::string const & url);

std::string GetFilePathByUrl(std::string const & url);
}  // namespace om::network::downloader
