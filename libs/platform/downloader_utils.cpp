#include "platform/downloader_utils.hpp"
#include "platform/platform.hpp"

#include "defines.hpp"

#include "base/file_name_utils.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file_utils.hpp"

#include "coding/url.hpp"

#include "base/string_utils.hpp"

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

  // The terrain blocks layout: "terrain/<version>/<name>.twm" (see GetTerrainDownloadUrl).
  if (urlComponents.size() == 3 && urlComponents[0] == TERRAIN_DIR)
  {
    uint64_t version;
    return strings::to_uint64(urlComponents[1], version) &&
           url::UrlDecode(urlComponents[2]).ends_with(TERRAIN_FILE_EXT);
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

  size_t count = 0;
  strings::Tokenize(url::UrlDecode(urlComponents.back()), ".", [&count](std::string_view) { ++count; });
  return count == 2;
}

std::string GetFilePathByUrl(std::string const & url)
{
  auto const urlComponents = strings::Tokenize(url, "/");
  CHECK_GREATER(urlComponents.size(), 2, (urlComponents));
  CHECK_LESS(urlComponents.size(), 5, (urlComponents));

  // The terrain blocks land into the <terrain>/<version>/ folder of the versioned URL
  // (cf. QueuedCountry::GetFileDownloadPath).
  if (urlComponents[0] == TERRAIN_DIR)
  {
    return base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, std::string(urlComponents[1]),
                          std::string(url::UrlDecode(urlComponents[2])) + READY_FILE_EXTENSION);
  }

  uint64_t dataVersion = 0;
  CHECK(strings::to_uint(urlComponents[1], dataVersion), ());

  std::string mwmFile = url::UrlDecode(urlComponents.back());
  // remove extension
  mwmFile = mwmFile.substr(0, mwmFile.find('.'));

  auto const fileType = urlComponents[0] == kDiffsPath ? MapFileType::Diff : MapFileType::Map;
  return platform::GetFileDownloadPath(dataVersion, mwmFile, fileType);
}

}  // namespace downloader
