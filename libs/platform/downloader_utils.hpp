#pragma once

#include <cstdint>
#include <string>

namespace downloader
{
// The terrain blocks CDN layout: terrain/<YYMMDD>/<name>.twm, the version comes from
// data/twm_grid.json (see storage terrain downloading).
// TODO(terrain): resolve the dynamic base url from the meta config, like for the maps.
std::string GetTerrainDownloadUrl(int64_t dataVersion, std::string const & fileName);
std::string GetFileDownloadUrl(std::string const & fileName, int64_t dataVersion, uint64_t diffVersion = 0);
bool IsUrlSupported(std::string const & url);
std::string GetFilePathByUrl(std::string const & url);
}  // namespace downloader
