#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"

#include "coding/zip_reader.hpp"

#include "base/scope_guard.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/iostream.hpp"


UNIT_TEST(KMZ_UnzipTest)
{
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

  auto kmlData = LoadKMLData(make_unique<FileReader>(kmlFile));
  TEST(kmlData != nullptr, ());

  TEST_EQUAL(files.size(), 6, ("KMZ file wrong number of files"));

  TEST_EQUAL(kmlData->m_bookmarks.size(), 6, ("Category wrong number of bookmarks"));

  {
    Bookmark const * bm = kmlData->m_bookmarks[0].get();
    TEST_EQUAL(bm->GetName(), ("Lahaina Breakwall"), ("KML wrong name!"));
    TEST_EQUAL(bm->GetType(), "placemark-red", ("KML wrong type!"));
    TEST_ALMOST_EQUAL_ULPS(bm->GetPivot().x, -156.6777046791284, ("KML wrong org x!"));
    TEST_ALMOST_EQUAL_ULPS(bm->GetPivot().y, 21.34256685860084, ("KML wrong org y!"));
    TEST_EQUAL(bm->GetScale(), -1, ("KML wrong scale!"));
  }
  {
    Bookmark const * bm = kmlData->m_bookmarks[1].get();
    TEST_EQUAL(bm->GetName(), ("Seven Sacred Pools, Kipahulu"), ("KML wrong name!"));
    TEST_EQUAL(bm->GetType(), "placemark-red", ("KML wrong type!"));
    TEST_ALMOST_EQUAL_ULPS(bm->GetPivot().x, -156.0405130750025, ("KML wrong org x!"));
    TEST_ALMOST_EQUAL_ULPS(bm->GetPivot().y, 21.12480639056074, ("KML wrong org y!"));
    TEST_EQUAL(bm->GetScale(), -1, ("KML wrong scale!"));
  }
}
