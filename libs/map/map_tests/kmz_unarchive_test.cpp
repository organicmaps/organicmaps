#include "testing/testing.hpp"

#include "map/bookmark.hpp"
#include "map/bookmark_helpers.hpp"

#include "platform/platform.hpp"

#include "base/scope_guard.hpp"

UNIT_TEST(KMZ_UnzipTest)
{
  /// @todo Should put somewhere in core? (like in BookmarkManager::PrepareToSaveBookmarks).
  TEST(Platform::MkDirChecked(GetBookmarksDirectory()), ());

  std::string const kmzFile = GetPlatform().TestsDataPathForFile("test_data/kml/test.kmz");
  auto const filePaths = GetKMLOrGPXFilesPathsToLoad(kmzFile);
  TEST_EQUAL(1, filePaths.size(), ());
  auto const & filePath = filePaths[0];
  TEST(filePath.ends_with("doc.kml"), (filePath));

  SCOPE_GUARD(fileGuard, std::bind(&base::DeleteFileX, filePath));

  auto const kmlData = LoadKmlFile(filePath, FileType::Kml);
  TEST(kmlData != nullptr, ());

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

UNIT_TEST(KMZ_RealLifeAreas)
{
  TEST(Platform::MkDirChecked(GetBookmarksDirectory()), ());

  // Dartmoor National Park Authority, "Backpack camping on Dartmoor commons", Open Government Licence v3.0:
  // https://www.arcgis.com/home/item.html?id=3d00736c2b2a44d4b59d58e7fccba07a
  std::string const kmzFile = GetPlatform().TestsDataPathForFile("test_data/kml/dartmoor_backpack_camping.kmz");
  auto const filePaths = GetKMLOrGPXFilesPathsToLoad(kmzFile);
  TEST_EQUAL(filePaths.size(), 1, ());
  SCOPE_GUARD(fileGuard, std::bind(&base::DeleteFileX, filePaths[0]));

  auto const kmlData = LoadKmlFile(filePaths[0], FileType::Kml);
  TEST(kmlData != nullptr, ());
  auto const & areas = kmlData->m_areasData;
  TEST_EQUAL(areas.size(), 70, ());
  size_t polygons = 0;
  size_t holes = 0;
  for (auto const & area : areas)
  {
    TEST_EQUAL(kml::GetDefaultStr(area.m_name), "Camping Permitted", ());
    TEST_EQUAL(area.m_style.m_fillColor, 0xBF7FBF97, ());
    TEST_EQUAL(area.m_style.m_outlineColor, 0x891289FF, ());
    TEST_EQUAL(area.m_style.m_outlineWidth, 2, ());
    polygons += area.m_polygons.size();
    for (auto const & polygon : area.m_polygons)
      holes += polygon.m_holes.size();
  }
  TEST_EQUAL(polygons, 76, ());
  TEST_EQUAL(holes, 18, ());
  // The largest area has a hole that touches itself at a vertex, which is valid for ArcGIS.
  TEST(std::ranges::any_of(areas, [](kml::AreaData const & area) { return area.m_polygons[0].m_holes.size() == 12; }),
       ());
}

UNIT_TEST(Multi_KML_KMZ_UnzipTest)
{
  TEST(Platform::MkDirChecked(GetBookmarksDirectory()), ());

  std::string const kmzFile = GetPlatform().TestsDataPathForFile("test_data/kml/BACRNKMZ.kmz");
  auto const filePaths = GetKMLOrGPXFilesPathsToLoad(kmzFile);
  SCOPE_GUARD(filesGuard, [&filePaths]()
  {
    for (auto const & path : filePaths)
      base::DeleteFileX(path);
  });

  base::StringIL expectedFileNames = {
      "BACRNKMZfilesCampgrounds 26may2022 green and tree icon",
      "BACRNKMZfilesIndoor Accommodations 26may2022 placemark purple and bed icon",
      "BACRNKMZfilesRoute 1 Canada - West-East Daily Segments",
      "BACRNKMZfilesRoute 2 Canada - West-East Daily Segments",
      "BACRNKMZfilesRoute Connector Canada - West-East Daily Segments",
      "BACRNKMZdoc",
  };
  TEST_EQUAL(expectedFileNames.size(), filePaths.size(), ());

  for (auto const & filePath : filePaths)
  {
    auto matched = false;
    for (auto const & expectedFileName : expectedFileNames)
    {
      matched = filePath.find(expectedFileName) != std::string::npos;
      if (matched)
        break;
    }
    TEST(matched, ("Unexpected file path: " + filePath));
  }
}
