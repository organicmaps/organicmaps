#include "../../testing/testing.hpp"

#include "../../map/framework.hpp"

#include "../../platform/platform.hpp"

#include "../../coding/internal/file_data.hpp"

#include "../../std/fstream.hpp"


namespace
{
char const * kmlString =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<kml xmlns=\"http://earth.google.com/kml/2.2\">"
    "<Document>"
      "<name>MapName</name>"
      "<description><![CDATA[MapDescription]]></description>"
      "<visibility>0</visibility>"
      "<Style id=\"placemark-blue\">"
        "<IconStyle>"
          "<Icon>"
            "<href>http://www.mapswithme.com/placemarks/placemark-blue.png</href>"
          "</Icon>"
        "</IconStyle>"
      "</Style>"
      "<Style id=\"placemark-brown\">"
        "<IconStyle>"
          "<Icon>"
            "<href>http://www.mapswithme.com/placemarks/placemark-brown.png</href>"
          "</Icon>"
        "</IconStyle>"
      "</Style>"
      "<Style id=\"placemark-green\">"
        "<IconStyle>"
          "<Icon>"
            "<href>http://www.mapswithme.com/placemarks/placemark-green.png</href>"
          "</Icon>"
        "</IconStyle>"
      "</Style>"
      "<Style id=\"placemark-orange\">"
        "<IconStyle>"
          "<Icon>"
            "<href>http://www.mapswithme.com/placemarks/placemark-orange.png</href>"
          "</Icon>"
        "</IconStyle>"
      "</Style>"
      "<Style id=\"placemark-pink\">"
        "<IconStyle>"
          "<Icon>"
            "<href>http://www.mapswithme.com/placemarks/placemark-pink.png</href>"
          "</Icon>"
        "</IconStyle>"
      "</Style>"
      "<Style id=\"placemark-purple\">"
        "<IconStyle>"
          "<Icon>"
            "<href>http://www.mapswithme.com/placemarks/placemark-purple.png</href>"
          "</Icon>"
        "</IconStyle>"
      "</Style>"
      "<Style id=\"placemark-red\">"
        "<IconStyle>"
          "<Icon>"
            "<href>http://www.mapswithme.com/placemarks/placemark-red.png</href>"
          "</Icon>"
        "</IconStyle>"
      "</Style>"
      "<Placemark>"
        "<name>Nebraska</name>"
        "<description><![CDATA[]]></description>"
        "<styleUrl>#placemark-red</styleUrl>"
        "<Point>"
          "<coordinates>-99.901810,41.492538,0.000000</coordinates>"
        "</Point>"
      "</Placemark>"
      "<Placemark>"
        "<name>Monongahela National Forest</name>"
        "<description><![CDATA[Huttonsville, WV 26273<br>]]></description>"
        "<styleUrl>#placemark-pink</styleUrl>"
        "<TimeStamp>"
          "<when>1986-08-12T07:10:43Z</when>"
        "</TimeStamp>"
        "<Point>"
          "<coordinates>-79.829674,38.627785,0.000000</coordinates>"
        "</Point>"
      "</Placemark>"
      "<Placemark>"
        "<name>From: Минск, Минская область, Беларусь</name>"
        "<description><![CDATA[]]></description>"
        "<styleUrl>#placemark-blue</styleUrl>"
        "<TimeStamp>"
          "<when>1998-03-03T03:04:48+01:30</when>"
        "</TimeStamp>"
        "<Point>"
          "<coordinates>27.566765,53.900047,0</coordinates>"
        "</Point>"
      "</Placemark>"
      "<Placemark>"
        "<name><![CDATA[<MWM & Sons>]]></name>"
        "<description><![CDATA[Amps & <brackets>]]></description>"
        "<styleUrl>#placemark-green</styleUrl>"
        "<TimeStamp>"
          "<when>2048 bytes in two kilobytes - some invalid timestamp</when>"
        "</TimeStamp>"
        "<Point>"
          "<coordinates>27.551532,53.89306</coordinates>"
        "</Point>"
      "</Placemark>"
    "</Document>"
    "</kml>";

  void CheckBookmarks(BookmarkCategory const & cat)
  {
    TEST_EQUAL(cat.GetBookmarksCount(), 4, ());

    Bookmark const * bm = cat.GetBookmark(0);
    TEST_EQUAL(bm->GetName(), "Nebraska", ());
    TEST_EQUAL(bm->GetType(), "placemark-red", ());
    TEST_EQUAL(bm->GetDescription(), "", ());
    TEST_EQUAL(bm->GetTimeStamp(), my::INVALID_TIME_STAMP, ());

    bm = cat.GetBookmark(1);
    TEST_EQUAL(bm->GetName(), "Monongahela National Forest", ());
    TEST_EQUAL(bm->GetType(), "placemark-pink", ());
    TEST_EQUAL(bm->GetDescription(), "Huttonsville, WV 26273<br>", ());
    TEST_EQUAL(bm->GetTimeStamp(), 524214643, ());

    bm = cat.GetBookmark(2);
    m2::PointD org = bm->GetOrg();
    TEST_ALMOST_EQUAL(MercatorBounds::XToLon(org.x), 27.566765, ());
    TEST_ALMOST_EQUAL(MercatorBounds::YToLat(org.y), 53.900047, ());
    TEST_EQUAL(bm->GetName(), "From: Минск, Минская область, Беларусь", ());
    TEST_EQUAL(bm->GetType(), "placemark-blue", ());
    TEST_EQUAL(bm->GetDescription(), "", ());
    TEST_EQUAL(bm->GetTimeStamp(), 888888888, ());

    bm = cat.GetBookmark(3);
    org = bm->GetOrg();
    TEST_ALMOST_EQUAL(MercatorBounds::XToLon(org.x), 27.551532, ());
    TEST_ALMOST_EQUAL(MercatorBounds::YToLat(org.y), 53.89306, ());
    TEST_EQUAL(bm->GetName(), "<MWM & Sons>", ());
    TEST_EQUAL(bm->GetDescription(), "Amps & <brackets>", ());
    TEST_EQUAL(bm->GetTimeStamp(), my::INVALID_TIME_STAMP, ());
  }
}

UNIT_TEST(Bookmarks_ImportKML)
{
  BookmarkCategory cat("Default");
  cat.LoadFromKML(new MemReader(kmlString, strlen(kmlString)));

  CheckBookmarks(cat);

  // Name should be overridden from file
  TEST_EQUAL(cat.GetName(), "MapName", ());
  TEST_EQUAL(cat.IsVisible(), false, ());
}

UNIT_TEST(Bookmarks_ExportKML)
{
  char const * BOOKMARKS_FILE_NAME = "UnitTestBookmarks.kml";

  BookmarkCategory cat("Default");
  cat.LoadFromKML(new MemReader(kmlString, strlen(kmlString)));
  CheckBookmarks(cat);

  TEST_EQUAL(cat.IsVisible(), false, ());
  // Change visibility
  cat.SetVisible(true);
  TEST_EQUAL(cat.IsVisible(), true, ());

  {
    ofstream of(BOOKMARKS_FILE_NAME);
    cat.SaveToKML(of);
  }

  cat.ClearBookmarks();
  TEST_EQUAL(cat.GetBookmarksCount(), 0, ());

  cat.LoadFromKML(new FileReader(BOOKMARKS_FILE_NAME));
  CheckBookmarks(cat);
  TEST_EQUAL(cat.IsVisible(), true, ());

  scoped_ptr<BookmarkCategory> cat2(BookmarkCategory::CreateFromKMLFile(BOOKMARKS_FILE_NAME));
  CheckBookmarks(*cat2);

  cat2->SaveToKMLFile();
  // old file should be deleted if we save bookmarks with new category name
  uint64_t dummy;
  TEST(!my::GetFileSize(BOOKMARKS_FILE_NAME, dummy), ());

  // MapName is the <name> tag in test kml data.
  string const catFileName = GetPlatform().WritableDir() + "MapName.kml";
  cat2.reset(BookmarkCategory::CreateFromKMLFile(catFileName));
  CheckBookmarks(*cat2);
  TEST(my::DeleteFileX(catFileName), ());
}

namespace
{
  // Call this function to delete test category files.
  void DeleteCategoryFiles()
  {
    string const path = GetPlatform().WritableDir();
    char const * arrFiles[] = { "cat1", "cat2", "cat3" };

    for (size_t i = 0; i < ARRAY_SIZE(arrFiles); ++i)
      FileWriter::DeleteFileX(path + arrFiles[i] + ".kml");
  }
}

UNIT_TEST(Bookmarks_Timestamp)
{
  time_t const timeStamp = time(0);
  m2::PointD const orgPoint(10, 10);
  Bookmark b1(orgPoint, "name", "type");
  b1.SetTimeStamp(timeStamp);
  TEST_NOT_EQUAL(b1.GetTimeStamp(), my::INVALID_TIME_STAMP, ());

  Framework fm;
  fm.AddBookmark("cat", b1);

  BookmarkAndCategory res = fm.GetBookmark(fm.GtoP(orgPoint), 1.0);
  Bookmark const * b2 = fm.GetBmCategory(res.first)->GetBookmark(res.second);
  TEST_NOT_EQUAL(b2->GetTimeStamp(), my::INVALID_TIME_STAMP, ());
  TEST_EQUAL(b2->GetTimeStamp(), timeStamp, ());

  // Replace/update bookmark
  Bookmark b3(orgPoint, "newName", "newType");
  b3.SetTimeStamp(12345);
  TEST_NOT_EQUAL(b3.GetTimeStamp(), b2->GetTimeStamp(), ());

  fm.AddBookmark("cat", b3);


  res = fm.GetBookmark(fm.GtoP(orgPoint), 1.0);
  Bookmark const * b4 = fm.GetBmCategory(res.first)->GetBookmark(res.second);
  TEST_EQUAL(b4->GetTimeStamp(), timeStamp, ());
}

UNIT_TEST(Bookmarks_Getting)
{
  Framework fm;
  fm.OnSize(800, 400);
  fm.ShowRect(m2::RectD(0, 0, 80, 40));

  // This is not correct because Framework::OnSize doesn't work until SetRenderPolicy is called.
  //TEST(m2::AlmostEqual(m2::PointD(400, 200), pixC), (pixC));

  BookmarkCategory const * c1 = fm.AddBookmark("cat1", Bookmark(m2::PointD(38, 20), "1", "placemark-red"));
  BookmarkCategory const * c2 = fm.AddBookmark("cat2", Bookmark(m2::PointD(41, 20), "2", "placemark-red"));
  BookmarkCategory const * c3 = fm.AddBookmark("cat3", Bookmark(m2::PointD(41, 40), "3", "placemark-red"));

  TEST_NOT_EQUAL(c1, c2, ());
  TEST_NOT_EQUAL(c2, c3, ());
  TEST_NOT_EQUAL(c1, c3, ());

  (void)fm.GetBmCategory("notExistingCat");
  TEST_EQUAL(fm.GetBmCategoriesCount(), 4, ());

  BookmarkAndCategory res = fm.GetBookmark(fm.GtoP(m2::PointD(40, 20)), 1.0);
  TEST(IsValid(res), ());
  TEST_EQUAL(res.second, 0, ());
  TEST_EQUAL(res.first, 1 , ());
  TEST_EQUAL(fm.GetBmCategory(res.first)->GetName(), "cat2", ());

  res = fm.GetBookmark(fm.GtoP(m2::PointD(0, 0)), 1.0);
  TEST(!IsValid(res), ());
  res = fm.GetBookmark(fm.GtoP(m2::PointD(800, 400)), 1.0);
  TEST(!IsValid(res), ());

  res = fm.GetBookmark(fm.GtoP(m2::PointD(41, 40)), 1.0);
  TEST(IsValid(res), ());
  TEST_EQUAL(res.first, 2, ());
  TEST_EQUAL(fm.GetBmCategory(res.first)->GetName(), "cat3", ());
  Bookmark const * bm = fm.GetBmCategory(res.first)->GetBookmark(res.second);
  TEST_EQUAL(bm->GetName(), "3", ());
  TEST_EQUAL(bm->GetType(), "placemark-red", ());

  // This one should replace previous bookmark
  BookmarkCategory const * c33 = fm.AddBookmark("cat3", Bookmark(m2::PointD(41, 40), "4", "placemark-blue"));

  TEST_EQUAL(c33, c3, ());

  res = fm.GetBookmark(fm.GtoP(m2::PointD(41, 40)), 1.0);
  TEST(IsValid(res), ());
  BookmarkCategory * cat = fm.GetBmCategory(res.first);
  TEST(cat, ());
  bm = cat->GetBookmark(res.second);
  TEST_EQUAL(bm->GetName(), "4", ());
  TEST_EQUAL(bm->GetType(), "placemark-blue", ());

  TEST_EQUAL(cat->GetBookmarksCount(), 1, ());

  cat->DeleteBookmark(0);
  TEST_EQUAL(cat->GetBookmarksCount(), 0, ());

  DeleteCategoryFiles();
}

UNIT_TEST(Bookmarks_AddressInfo)
{
  // Maps added in constructor (we need minsk-pass.mwm)
  Framework fm;

  Framework::AddressInfo info;
  fm.GetAddressInfo(m2::PointD(MercatorBounds::LonToX(27.5556), MercatorBounds::LatToY(53.8963)), info);

  TEST_EQUAL(info.m_street, "ул. Карла Маркса", ());
  TEST_EQUAL(info.m_house, "10", ());
  TEST_EQUAL(info.m_name, "Библос", ());
  TEST_EQUAL(info.m_types.size(), 1, ());

  // assume that developers have English or Russian system language :)
  TEST(info.m_types[0] == "cafe" || info.m_types[0] == "кафе", (info.m_types[0]));
}

UNIT_TEST(Bookmarks_IllegalFileName)
{
  char const * arrIllegal[] = { "?", "?|", "\"x", "|x:", "x<>y", "xy*"};
  char const * arrLegal[] =   { "",  "",   "x",   "x",   "xy",   "xy"};

  for (size_t i = 0; i < ARRAY_SIZE(arrIllegal); ++i)
  {
    string const name = BookmarkCategory::RemoveInvalidSymbols(arrIllegal[i]);

    if (strlen(arrLegal[i]) == 0)
    {
      TEST_EQUAL("Bookmarks", name, ());
    }
    else
    {
      TEST_EQUAL(arrLegal[i], name, ());
    }
  }
}

UNIT_TEST(Bookmarks_UniqueFileName)
{
  string const BASE = "SomeUniqueFileName";
  string const FILENAME = BASE + ".kml";

  {
    FileWriter file(FILENAME);
    file.Write(FILENAME.data(), FILENAME.size());
  }

  string gen = BookmarkCategory::GenerateUniqueFileName("", BASE);
  TEST_NOT_EQUAL(gen, FILENAME, ());
  TEST_EQUAL(gen, BASE + "1.kml", ());

  string const FILENAME1 = gen;
  {
    FileWriter file(FILENAME1);
    file.Write(FILENAME1.data(), FILENAME1.size());
  }
  gen = BookmarkCategory::GenerateUniqueFileName("", BASE);
  TEST_NOT_EQUAL(gen, FILENAME, ());
  TEST_NOT_EQUAL(gen, FILENAME1, ());
  TEST_EQUAL(gen, BASE + "2.kml", ());

  FileWriter::DeleteFileX(FILENAME);
  FileWriter::DeleteFileX(FILENAME1);

  gen = BookmarkCategory::GenerateUniqueFileName("", BASE);
  TEST_EQUAL(gen, FILENAME, ());
}

UNIT_TEST(Bookmarks_AddingMoving)
{
  Framework fm;
  fm.OnSize(800, 400);
  fm.ShowRect(m2::RectD(0, 0, 80, 40));

  string const categoryOne = "cat1";
  string const categoryTwo = "cat2";
  m2::PointD const globalPoint = m2::PointD(40, 20);
  m2::PointD const pixelPoint = fm.GtoP(globalPoint);

  BookmarkCategory const * c1 = fm.AddBookmark(categoryOne, Bookmark(globalPoint, "name", "placemark-red"));
  BookmarkAndCategory res = fm.GetBookmark(pixelPoint, 1.0);
  TEST(IsValid(res), ());
  TEST_EQUAL(res.second, 0, ());
  TEST_EQUAL(res.first, 0, ());
  TEST_EQUAL(fm.GetBmCategory(res.first)->GetName(), categoryOne, ());

  // Edit the name and type of bookmark
  BookmarkCategory const * c11 = fm.AddBookmark(categoryOne, Bookmark(globalPoint, "name2", "placemark-blue"));
  TEST_EQUAL(c1, c11, ());
  res = fm.GetBookmark(pixelPoint, 1.0);
  TEST(IsValid(res), ());
  TEST_EQUAL(res.second, 0, ());
  TEST_EQUAL(res.first, 0, ());
  TEST_EQUAL(fm.GetBmCategory(res.first)->GetName(), categoryOne, ());
  Bookmark const * pBm = fm.GetBmCategory(res.first)->GetBookmark(res.second);
  TEST_EQUAL(pBm->GetName(), "name2", ());
  TEST_EQUAL(pBm->GetType(), "placemark-blue", ());

  // Edit name, type and category of bookmark
  BookmarkCategory const * c2 = fm.AddBookmark(categoryTwo, Bookmark(globalPoint, "name3", "placemark-green"));
  TEST_NOT_EQUAL(c1, c2, ());
  TEST_EQUAL(fm.GetBmCategoriesCount(), 2, ());
  res = fm.GetBookmark(pixelPoint, 1.0);
  TEST(IsValid(res), ());
  TEST_EQUAL(res.second, 0, ());
  TEST_EQUAL(res.first, 1, ());
  TEST_EQUAL(fm.GetBmCategory(res.first)->GetName(), categoryTwo, ());
  TEST_EQUAL(fm.GetBmCategory(categoryOne)->GetBookmarksCount(), 0,
             ("Bookmark wasn't moved from one category to another"));
  pBm = fm.GetBmCategory(res.first)->GetBookmark(res.second);
  TEST_EQUAL(pBm->GetName(), "name3", ());
  TEST_EQUAL(pBm->GetType(), "placemark-green", ());

  DeleteCategoryFiles();
}

namespace
{
char const * kmlString2 =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<kml xmlns=\"http://earth.google.com/kml/2.1\">"
    "<Document>"
     "<name>busparkplatz</name>"
     "<Folder>"
      "<name>Waypoint</name>"
      "<Style id=\"poiIcon37\">"
       "<IconStyle>"
       "<scale>1</scale>"
       "<Icon><href>http://maps.google.com/mapfiles/kml/shapes/placemark_circle.png</href></Icon>"
       "<hotSpot x=\"0.5\" y=\"0\" xunits=\"fraction\" yunits=\"fraction\"/>"
       "</IconStyle>"
      "</Style>"
      "<Placemark>"
       "<name>[P] Silvrettastrae[Bieler Hhe]</name>"
       "<description></description>"
       "<styleUrl>#poiIcon37</styleUrl>"
       "<Point>"
        "<coordinates>10.09237,46.91741,0</coordinates>"
       "</Point>"
      "</Placemark>"
    "</Folder>"
   "</Document>"
   "</kml>";
}

UNIT_TEST(Bookmarks_InnerFolder)
{
  BookmarkCategory cat("Default");
  cat.LoadFromKML(new MemReader(kmlString2, strlen(kmlString2)));

  TEST_EQUAL(cat.GetBookmarksCount(), 1, ());
}
