#include "network/downloader/utils.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <gtest/gtest.h>

namespace om::network::downloader
{
TEST(DownloaderUtils, GetFilePathByUrl)
{
  {
    std::string const mwmName = "Luna";
    std::string const fileName = platform::GetFileName(mwmName, MapFileType::Map);
    int64_t const dataVersion = version::FOR_TESTING_MWM1;
    int64_t const diffVersion = 0;
    MapFileType const fileType = MapFileType::Map;

    auto const path = platform::GetFileDownloadPath(dataVersion, mwmName, fileType);

    auto const url = GetFileDownloadUrl(fileName, dataVersion, diffVersion);
    auto const resultPath = GetFilePathByUrl(url);

    EXPECT_EQ(path, resultPath);
  }
  {
    std::string const mwmName = "Luna";
    std::string const fileName = platform::GetFileName(mwmName, MapFileType::Diff);
    int64_t const dataVersion = version::FOR_TESTING_MWM2;
    int64_t const diffVersion = version::FOR_TESTING_MWM1;
    MapFileType const fileType = MapFileType::Diff;

    auto const path = platform::GetFileDownloadPath(dataVersion, mwmName, fileType);

    auto const url = GetFileDownloadUrl(fileName, dataVersion, diffVersion);
    auto const resultPath = GetFilePathByUrl(url);

    EXPECT_EQ(path, resultPath);
  }

  EXPECT_EQ(GetFilePathByUrl("/maps/220314/Belarus_Brest Region.mwm"),
            base::JoinPath(GetPlatform().WritableDir(), "220314/Belarus_Brest Region.mwm.ready"));
}

TEST(DownloaderUtils, IsUrlSupported)
{
  std::string const mwmName = "Luna";

  std::string fileName = platform::GetFileName(mwmName, MapFileType::Map);
  int64_t dataVersion = version::FOR_TESTING_MWM1;
  int64_t diffVersion = 0;

  auto url = GetFileDownloadUrl(fileName, dataVersion, diffVersion);
  EXPECT_TRUE(IsUrlSupported(url));
  EXPECT_TRUE(IsUrlSupported("maps/991215/Luna.mwm"));
  EXPECT_TRUE(IsUrlSupported("maps/0/Luna.mwm"));
  EXPECT_FALSE(IsUrlSupported("maps/x/Luna.mwm"));
  EXPECT_FALSE(IsUrlSupported("macarena/0/Luna.mwm"));
  EXPECT_FALSE(IsUrlSupported("/hack/maps/0/Luna.mwm"));
  EXPECT_FALSE(IsUrlSupported("0/Luna.mwm"));
  EXPECT_FALSE(IsUrlSupported("maps/0/Luna"));
  EXPECT_FALSE(IsUrlSupported("0/Luna.mwm"));
  EXPECT_FALSE(IsUrlSupported("Luna.mwm"));
  EXPECT_FALSE(IsUrlSupported("Luna"));

  fileName = platform::GetFileName(mwmName, MapFileType::Diff);
  diffVersion = version::FOR_TESTING_MWM1;
  url = GetFileDownloadUrl(fileName, dataVersion, diffVersion);
  EXPECT_TRUE(IsUrlSupported(url));
  EXPECT_TRUE(IsUrlSupported("diffs/991215/991215/Luna.mwmdiff"));
  EXPECT_TRUE(IsUrlSupported("diffs/0/0/Luna.mwmdiff"));
  EXPECT_FALSE(IsUrlSupported("diffs/x/0/Luna.mwmdiff"));
  EXPECT_FALSE(IsUrlSupported("diffs/0/x/Luna.mwmdiff"));
  EXPECT_FALSE(IsUrlSupported("diffs/x/x/Luna.mwmdiff"));
  EXPECT_FALSE(IsUrlSupported("beefs/0/0/Luna.mwmdiff"));
  EXPECT_FALSE(IsUrlSupported("diffs/0/0/Luna.mwmdiff.f"));
  EXPECT_FALSE(IsUrlSupported("maps/diffs/0/0/Luna.mwmdiff"));
  EXPECT_FALSE(IsUrlSupported("diffs/0/0/Luna"));
  EXPECT_FALSE(IsUrlSupported("0/0/Luna.mwmdiff"));
  EXPECT_FALSE(IsUrlSupported("diffs/0/Luna.mwmdiff"));
  EXPECT_FALSE(IsUrlSupported("diffs/0"));
  EXPECT_FALSE(IsUrlSupported("diffs/0/Luna.mwmdiff"));
  EXPECT_FALSE(IsUrlSupported("diffs/Luna.mwmdiff"));
  EXPECT_FALSE(IsUrlSupported("Luna.mwmdiff"));
  EXPECT_FALSE(IsUrlSupported("Luna"));
}
}  // namespace om::network::downloader
