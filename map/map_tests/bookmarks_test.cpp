#include "../../testing/testing.hpp"

#include "../../map/framework.hpp"

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
        "<Point>"
          "<coordinates>-79.829674,38.627785,0.000000</coordinates>"
        "</Point>"
      "</Placemark>"
      "<Placemark>"
        "<name>From: Минск, Минская область, Беларусь</name>"
        "<description><![CDATA[]]></description>"
        "<styleUrl>#placemark-blue</styleUrl>"
        "<Point>"
          "<coordinates>27.566765,53.900047,0.000000</coordinates>"
        "</Point>"
      "</Placemark>"
    "</Document>"
    "</kml>";

  void CheckBookmarks(BookmarkCategory const & cat)
  {
    TEST_EQUAL(cat.GetBookmarksCount(), 3, ());

    Bookmark const * bm = cat.GetBookmark(0);
    TEST_EQUAL(bm->GetName(), "Nebraska", ());
    TEST_EQUAL(bm->GetType(), "placemark-red", ());

    bm = cat.GetBookmark(1);
    TEST_EQUAL(bm->GetName(), "Monongahela National Forest", ());
    TEST_EQUAL(bm->GetType(), "placemark-pink", ());

    bm = cat.GetBookmark(2);
    m2::PointD const org = bm->GetOrg();
    TEST_ALMOST_EQUAL(MercatorBounds::XToLon(org.x), 27.566765, ());
    TEST_ALMOST_EQUAL(MercatorBounds::YToLat(org.y), 53.900047, ());
    TEST_EQUAL(bm->GetType(), "placemark-blue", ());
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

  BookmarkCategory * cat2 = BookmarkCategory::CreateFromKMLFile(BOOKMARKS_FILE_NAME);
  CheckBookmarks(*cat2);
  FileWriter::DeleteFileX(BOOKMARKS_FILE_NAME);

  cat2->SaveToKMLFileAtPath("./");
  delete cat2;

  cat2 = BookmarkCategory::CreateFromKMLFile(BOOKMARKS_FILE_NAME);
  CheckBookmarks(*cat2);
  FileWriter::DeleteFileX(BOOKMARKS_FILE_NAME);
}

UNIT_TEST(Bookmarks_Getting)
{
  Framework fm;
  fm.OnSize(800, 400);
  fm.ShowRect(m2::RectD(0, 0, 80, 40));

  m2::PointD const pixC = fm.GtoP(m2::PointD(40, 20));

  // This is not correct because Framework::OnSize doesn't work until SetRenderPolicy is called.
  //TEST(m2::AlmostEqual(m2::PointD(400, 200), pixC), (pixC));

  fm.AddBookmark("cat1", Bookmark(m2::PointD(38, 20), "1", "placemark-red"));
  fm.AddBookmark("cat2", Bookmark(m2::PointD(41, 20), "2", "placemark-red"));
  fm.AddBookmark("cat3", Bookmark(m2::PointD(41, 40), "3", "placemark-red"));

  BookmarkAndCategory res = fm.GetBookmark(pixC, 1.0);
  TEST(res.second != 0, ());
  TEST(res.first != 0, ());
  TEST_EQUAL(res.second->GetName(), "2", ());
  TEST_EQUAL(res.first->GetName(), "cat2" , ());

  res = fm.GetBookmark(m2::PointD(0, 0));
  TEST(res.first == 0, ());
  TEST(res.second == 0, ());
  res = fm.GetBookmark(m2::PointD(800, 400));
  TEST(res.first == 0, ());
  TEST(res.second == 0, ());
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
