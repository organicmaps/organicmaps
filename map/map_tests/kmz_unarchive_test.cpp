#include "testing/testing.hpp"

#include "map/bookmark_helpers.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"


UNIT_TEST(KMZ_UnzipTest)
{
  std::string const kmzFile = GetPlatform().TestsDataPathForFile("test.kmz");
  std::string const filePath = GetKMLPath(kmzFile);
  TEST(!filePath.empty(), ());
  std::string const pngDir = base::JoinPath(GetBookmarksDirectory(), "files");

  auto fileGuard = base::make_scope_guard([&filePath, &pngDir]()
  {
    TEST(base::DeleteFileX(filePath), ());
    // Cleanup 'files' folder from "test.kmz".
    TEST(Platform::RmDirRecursively(pngDir), ());
  });

  {
    Platform::FilesList filesList;
    Platform::GetFilesRecursively(pngDir, filesList);
    TEST_EQUAL(filesList.size(), 4, ());
    TEST(strings::EndsWith(filePath, "doc.kml"), (filePath));
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
