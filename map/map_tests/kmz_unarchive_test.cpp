#include "testing/testing.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/framework.hpp"
#include "map/user_mark_id_storage.hpp"

#include "platform/platform.hpp"

#include "coding/zip_reader.hpp"

#include "base/scope_guard.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/iostream.hpp"


UNIT_TEST(KMZ_UnzipTest)
{
  PersistentIdStorage::Instance().EnableTestMode(true);

  string const kmzFile = GetPlatform().TestsDataPathForFile("test.kmz");
  ZipFileReader::FileListT files;
  ZipFileReader::FilesList(kmzFile, files);

  bool isKMLinZip = false;

  for (size_t i = 0; i < files.size(); ++i)
  {
    if (files[i].first == "doc.kml")
    {
      isKMLinZip = true;
      break;
    }
  }
  TEST(isKMLinZip, ("No KML file in KMZ"));

  string const kmlFile = GetPlatform().WritablePathForFile("newKml.kml");
  MY_SCOPE_GUARD(fileGuard, bind(&FileWriter::DeleteFileX, kmlFile));
  ZipFileReader::UnzipFile(kmzFile, "doc.kml", kmlFile);

  auto kmlData = LoadKmlData(FileReader(kmlFile), false /* useBinary */);
  TEST(kmlData != nullptr, ());

  TEST_EQUAL(files.size(), 6, ("KMZ file wrong number of files"));

  TEST_EQUAL(kmlData->m_bookmarksData.size(), 6, ("Category wrong number of bookmarks"));

  {
    Bookmark const bm(std::move(kmlData->m_bookmarksData[0]));
    TEST_EQUAL(bm.GetName(), ("Lahaina Breakwall"), ("KML wrong name!"));
    TEST_EQUAL(bm.GetColor(), kml::PredefinedColor::Red, ("KML wrong type!"));
    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().x, -156.6777046791284, ("KML wrong org x!"));
    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().y, 21.34256685860084, ("KML wrong org y!"));
    TEST_EQUAL(bm.GetScale(), 0, ("KML wrong scale!"));
  }
  {
    Bookmark const bm(std::move(kmlData->m_bookmarksData[1]));
    TEST_EQUAL(bm.GetName(), ("Seven Sacred Pools, Kipahulu"), ("KML wrong name!"));
    TEST_EQUAL(bm.GetColor(), kml::PredefinedColor::Red, ("KML wrong type!"));
    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().x, -156.0405130750025, ("KML wrong org x!"));
    TEST_ALMOST_EQUAL_ULPS(bm.GetPivot().y, 21.12480639056074, ("KML wrong org y!"));
    TEST_EQUAL(bm.GetScale(), 0, ("KML wrong scale!"));
  }
}
