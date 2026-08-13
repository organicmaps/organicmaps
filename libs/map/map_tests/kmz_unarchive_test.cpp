#include "testing/testing.hpp"

#include "map/bookmark.hpp"
#include "map/bookmark_helpers.hpp"

#include "platform/platform.hpp"

#include "base/scope_guard.hpp"

UNIT_TEST(KMZ_UnzipTest)
{
  TEST(Platform::MkDirChecked(GetBookmarksDirectory()), ());

  std::string const kmzFile = GetPlatform().TestsDataPathForFile("test_data/kml/test.kmz");
  auto result = LoadBookmarkFileForImport(kmzFile);
  TEST(result.m_failedFileNames.empty(), (result.m_failedFileNames));
  TEST_EQUAL(1, result.m_kmlData.size(), ());
  auto const & filePath = result.m_kmlData.front().first;
  TEST(filePath.ends_with(kKmzIndexFileName), (filePath));

  SCOPE_GUARD(fileGuard, std::bind(&base::DeleteFileX, filePath));

  auto & kmlData = result.m_kmlData.front().second;

  TEST_EQUAL(kmlData->m_bookmarksData.size(), 6, ("Category wrong number of bookmarks"));

  {
    Bookmark const bm(std::move(kmlData->m_bookmarksData[0]));
    TEST_EQUAL(kml::GetDefaultStr(bm.GetName()), ("Lahaina Breakwall"), ("KML wrong name!"));
    TEST_EQUAL(bm.GetColor(), kml::PredefinedColor::Red, ("KML wrong type!"));
    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().x, -156.6777046791284, ("KML wrong org x!"));
    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().y, 21.34256685860084, ("KML wrong org y!"));
    TEST_EQUAL(bm.GetScale(), 0, ("KML wrong scale!"));
  }
  {
    Bookmark const bm(std::move(kmlData->m_bookmarksData[1]));
    TEST_EQUAL(kml::GetDefaultStr(bm.GetName()), ("Seven Sacred Pools, Kipahulu"), ("KML wrong name!"));
    TEST_EQUAL(bm.GetColor(), kml::PredefinedColor::Red, ("KML wrong type!"));
    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().x, -156.0405130750025, ("KML wrong org x!"));
    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().y, 21.12480639056074, ("KML wrong org y!"));
    TEST_EQUAL(bm.GetScale(), 0, ("KML wrong scale!"));
  }
}

UNIT_TEST(Multi_KML_KMZ_UnzipTest)
{
  TEST(Platform::MkDirChecked(GetBookmarksDirectory()), ());

  std::string const kmzFile = GetPlatform().TestsDataPathForFile("test_data/kml/BACRNKMZ.kmz");
  auto const result = LoadBookmarkFileForImport(kmzFile);
  TEST(result.m_failedFileNames.empty(), (result.m_failedFileNames));
  SCOPE_GUARD(filesGuard, [&result]()
  {
    for (auto const & file : result.m_kmlData)
      base::DeleteFileX(file.first);
  });

  base::StringIL expectedFileNames = {
      "BACRNKMZfilesCampgrounds 26may2022 green and tree icon",
      "BACRNKMZfilesIndoor Accommodations 26may2022 placemark purple and bed icon",
      "BACRNKMZfilesRoute 1 Canada - West-East Daily Segments",
      "BACRNKMZfilesRoute 2 Canada - West-East Daily Segments",
      "BACRNKMZfilesRoute Connector Canada - West-East Daily Segments",
  };
  TEST_EQUAL(expectedFileNames.size(), result.m_kmlData.size(), ());

  for (auto const & file : result.m_kmlData)
  {
    auto matched = false;
    for (auto const & expectedFileName : expectedFileNames)
    {
      matched = file.first.find(expectedFileName) != std::string::npos;
      if (matched)
        break;
    }
    TEST(matched, ("Unexpected file path: " + file.first));
  }
}

UNIT_TEST(KMZ_UnzipFailureContainsFileNameTest)
{
  auto const filePath = base::JoinPath(GetPlatform().WritableDir(), "missing_bookmarks.kmz");
  base::DeleteFileX(filePath);

  auto const result = LoadBookmarkFileForImport(filePath);
  TEST(result.m_kmlData.empty(), (result.m_kmlData.size()));
  TEST_EQUAL(1, result.m_failedFileNames.size(), (result.m_failedFileNames));
  TEST_EQUAL("missing_bookmarks.kmz", result.m_failedFileNames[0], ());
}
