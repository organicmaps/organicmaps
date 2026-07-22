#pragma once

#include <cstdint>
#include <string>

namespace downloader
{
// The terrain blocks CDN layout (see storage terrain downloading).
// TODO(terrain): move to a versioned layout together with the maps mirrors.
std::string GetTerrainDownloadUrl(std::string const & fileName);
std::string GetFileDownloadUrl(std::string const & fileName, int64_t dataVersion, uint64_t diffVersion = 0);
bool IsUrlSupported(std::string const & url);
std::string GetFilePathByUrl(std::string const & url);
}  // namespace downloader
