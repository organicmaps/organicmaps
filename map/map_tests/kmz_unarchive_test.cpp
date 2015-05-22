#include "testing/testing.hpp"

#include "coding/zip_reader.hpp"
#include "map/framework.hpp"
#include "platform/platform.hpp"
#include "base/scope_guard.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/iostream.hpp"

UNIT_TEST(Open_KMZ_Test)
{
  string const KMZFILE = GetPlatform().SettingsPathForFile("test.kmz");
  ZipFileReader::FileListT files;
  ZipFileReader::FilesList(KMZFILE, files);

  bool isKMLinZip = false;

  for (int i = 0; i < files.size();++i)
  {
    if (files[i].first == "doc.kml")
    {
      isKMLinZip = true;
      break;
    }
  }
  TEST(isKMLinZip, ("No KML file in KMZ"));

  string const KMLFILE = GetPlatform().SettingsPathForFile("newKml.kml");
  MY_SCOPE_GUARD(fileGuard, bind(&FileWriter::DeleteFileX, KMLFILE));
  ZipFileReader::UnzipFile(KMZFILE, "doc.kml", KMLFILE);

  Framework framework;
  BookmarkCategory cat("Default", framework);
  TEST(cat.LoadFromKML(new FileReader(KMLFILE)), ());

  TEST_EQUAL(files.size(), 6, ("KMZ file wrong number of files"));

  TEST_EQUAL(cat.GetBookmarksCount(), 6, ("Category wrong number of bookmarks"));

  {
    Bookmark const * bm = cat.GetBookmark(5);
    TEST_EQUAL(bm->GetName(), ("Lahaina Breakwall"), ("KML wrong name!"));
    TEST_EQUAL(bm->GetType(), "placemark-red", ("KML wrong type!"));
    TEST_ALMOST_EQUAL_ULPS(bm->GetOrg().x, -156.6777046791284, ("KML wrong org x!"));
    TEST_ALMOST_EQUAL_ULPS(bm->GetOrg().y, 21.34256685860084, ("KML wrong org y!"));
    TEST_EQUAL(bm->GetScale(), -1, ("KML wrong scale!"));
  }
  {
    Bookmark const * bm = cat.GetBookmark(4);
    TEST_EQUAL(bm->GetName(), ("Seven Sacred Pools, Kipahulu"), ("KML wrong name!"));
    TEST_EQUAL(bm->GetType(), "placemark-red", ("KML wrong type!"));
    TEST_ALMOST_EQUAL_ULPS(bm->GetOrg().x, -156.0405130750025, ("KML wrong org x!"));
    TEST_ALMOST_EQUAL_ULPS(bm->GetOrg().y, 21.12480639056074, ("KML wrong org y!"));
    TEST_EQUAL(bm->GetScale(), -1, ("KML wrong scale!"));
  }
}
