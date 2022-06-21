#include "testing/testing.hpp"

#include "map/bookmark_helpers.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "base/file_name_utils.hpp"


UNIT_TEST(KMZ_UnzipTest)
{
  std::string const kmzFile = GetPlatform().TestsDataPathForFile("kml_test_data/test.kmz");

  // Temporary path for GetBookmarksDirectory.
  WritableDirChanger dirGuard("kmz_test", true /* setSettingsDir */);

  std::string const filePath = GetKMLorGPXPath(kmzFile);
  TEST(!filePath.empty(), ());
  TEST(strings::EndsWith(filePath, "doc.kml"), (filePath));

  {
    Platform::FilesList filesList;
    Platform::GetFilesRecursively(base::JoinPath(GetBookmarksDirectory(), "files"), filesList);
    TEST_EQUAL(filesList.size(), 4, ());
  }

  auto const kmlData = LoadKmlFile(filePath, KmlFileType::Text);
  TEST(kmlData != nullptr, ());

  TEST_EQUAL(kmlData->m_bookmarksData.size(), 6, ("Category wrong number of bookmarks"));

  auto const checkUserSymbol = [](Bookmark const & bm, std::string const & symbolPath)
  {
    TEST_EQUAL(bm.GetColor(), kml::PredefinedColor::None, ());

    auto const symbolNames = bm.GetSymbolNames();
    TEST(!symbolNames->m_pathPrefix.empty(), ());
    TEST(!symbolNames->m_zoomInfo.empty(), ());
    TEST_EQUAL(symbolNames->m_zoomInfo.begin()->second, symbolPath, ());
  };

  {
    Bookmark const bm(std::move(kmlData->m_bookmarksData[0]));
    TEST_EQUAL(kml::GetDefaultStr(bm.GetName()), ("Lahaina Breakwall"), ());

    checkUserSymbol(bm, "files/icon_surfing.png");

    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().x, -156.6777046791284, ());
    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().y, 21.34256685860084, ());
    TEST_EQUAL(bm.GetScale(), 0, ());
  }
  {
    Bookmark const bm(std::move(kmlData->m_bookmarksData[1]));
    TEST_EQUAL(kml::GetDefaultStr(bm.GetName()), ("Seven Sacred Pools, Kipahulu"), ());

    checkUserSymbol(bm, "files/icon_surfing.png");

    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().x, -156.0405130750025, ());
    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().y, 21.12480639056074, ());
    TEST_EQUAL(bm.GetScale(), 0, ());
  }
}
