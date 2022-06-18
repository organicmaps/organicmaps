#include "testing/testing.hpp"

#include "map/bookmark_helpers.hpp"

#include "platform/platform.hpp"

#include "base/scope_guard.hpp"


UNIT_TEST(KMZ_UnzipTest)
{
  std::string const kmzFile = GetPlatform().TestsDataPathForFile("test.kmz");
  std::string const filePath = GetKMLPath(kmzFile);

  TEST(!filePath.empty(), ());
  SCOPE_GUARD(fileGuard, std::bind(&base::DeleteFileX, filePath));

  TEST(strings::EndsWith(filePath, "doc.kml"), (filePath));

  auto const kmlData = LoadKmlFile(filePath, KmlFileType::Text);
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
