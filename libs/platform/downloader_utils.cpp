#include "platform/downloader_utils.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file_utils.hpp"

#include "coding/url.hpp"

#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

namespace
{
std::string const kMapsPath = "maps";
std::string const kDiffsPath = "diffs";
}  // namespace

namespace downloader
{

std::string GetTerrainDownloadUrl(int64_t dataVersion, std::string const & fileName)
{
  return url::Join(TERRAIN_DIR, strings::to_string(dataVersion), url::UrlEncode(fileName));
}

std::string GetFileDownloadUrl(std::string const & fileName, int64_t dataVersion, uint64_t diffVersion /* = 0 */)
{
  if (diffVersion == 0)
    return url::Join(kMapsPath, strings::to_string(dataVersion), url::UrlEncode(fileName));

  return url::Join(kDiffsPath, strings::to_string(dataVersion), strings::to_string(diffVersion),
                   url::UrlEncode(fileName));
}

bool IsUrlSupported(std::string const & url)
{
  auto const urlComponents = strings::Tokenize(url, "/");
  if (urlComponents.empty())
    return false;

  auto const fileName = url::UrlDecode(urlComponents.back());
  auto const dot = fileName.find('.');
  if (dot == std::string::npos || dot == 0 || dot + 1 == fileName.size() || dot != fileName.rfind('.') ||
      fileName.find_first_of("/\\") != std::string::npos || fileName.find('\0') != std::string::npos)
    return false;

  // The terrain blocks layout: "terrain/<version>/<name>.twm" (see GetTerrainDownloadUrl).
  if (urlComponents.size() == 3 && urlComponents[0] == TERRAIN_DIR)
  {
    uint64_t version;
    return strings::to_uint(urlComponents[1], version) && fileName.ends_with(TERRAIN_FILE_EXT);
  }

  if (urlComponents[0] != kMapsPath && urlComponents[0] != kDiffsPath)
    return false;

  if (urlComponents[0] == kMapsPath && urlComponents.size() != 3)
    return false;

  if (urlComponents[0] == kDiffsPath && urlComponents.size() != 4)
    return false;

  uint64_t dataVersion = 0;
  if (!strings::to_uint(urlComponents[1], dataVersion))
    return false;

  if (urlComponents[0] == kDiffsPath)
  {
    uint64_t diffVersion = 0;
    if (!strings::to_uint(urlComponents[2], diffVersion))
      return false;
  }

  return true;
}

std::string GetFilePathByUrl(std::string const & url)
{
  auto const urlComponents = strings::Tokenize(url, "/");
  CHECK_GREATER(urlComponents.size(), 2, (urlComponents));
  CHECK_LESS(urlComponents.size(), 5, (urlComponents));

  uint64_t dataVersion = 0;
  CHECK(strings::to_uint(urlComponents[1], dataVersion), ());

  auto fileName = url::UrlDecode(urlComponents.back());
  base::GetNameWithoutExt(fileName);

  auto const fileType = urlComponents[0] == TERRAIN_DIR ? MapFileType::Terrain
                      : urlComponents[0] == kDiffsPath  ? MapFileType::Diff
                                                        : MapFileType::Map;
  return platform::GetFileDownloadPath(dataVersion, fileName, fileType);
}

}  // namespace downloader
