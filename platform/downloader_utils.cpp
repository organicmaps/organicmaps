#include "platform/downloader_utils.hpp"

#include "platform/country_defines.hpp"
#include "platform/country_file.hpp"
#include "platform/local_country_file_utils.hpp"

#include "coding/url.hpp"

#include "base/string_utils.hpp"

#include "std/target_os.hpp"

#include <sstream>

namespace downloader
{
std::string GetFileDownloadUrl(std::string const & fileName, int64_t dataVersion,
                               uint64_t diffVersion)
{
  std::ostringstream url;
  if (diffVersion != 0)
    url << "diffs/" << dataVersion << "/" << diffVersion;
  else
    url << OMIM_OS_NAME "/" << dataVersion;

  return url::Join(url.str(), url::UrlEncode(fileName));
}

std::string GetFilePathByUrl(std::string const & url)
{
  auto const urlComponents = strings::Tokenize(url, "/");
  CHECK_GREATER(urlComponents.size(), 1, ());

  int64_t dataVersion = 0;
  CHECK(strings::to_int64(urlComponents[1], dataVersion), ());

  auto const countryComponents = strings::Tokenize(url::UrlDecode(urlComponents.back()), ".");
  CHECK(!urlComponents.empty(), ());

  auto const fileType = urlComponents[0] == "diffs" ? MapFileType::Diff : MapFileType::Map;

  return platform::GetFileDownloadPath(dataVersion, platform::CountryFile(countryComponents[0]),
                                       fileType);
}
}  // namespace downloader
