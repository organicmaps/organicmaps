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
      "<Style id=\"style2\">"
        "<IconStyle>"
          "<Icon>"
            "<href>http://maps.gstatic.com/mapfiles/ms2/micons/blue-dot.png</href>"
          "</Icon>"
        "</IconStyle>"
      "</Style>"
      "<Style id=\"style3\">"
        "<IconStyle>"
          "<Icon>"
            "<href>http://maps.gstatic.com/mapfiles/ms2/micons/blue-dot.png</href>"
          "</Icon>"
        "</IconStyle>"
      "</Style>"
      "<Placemark>"
        "<name>Nebraska</name>"
        "<description><![CDATA[]]></description>"
        "<styleUrl>#style2</styleUrl>"
        "<Point>"
          "<coordinates>-99.901810,41.492538,0.000000</coordinates>"
        "</Point>"
      "</Placemark>"
      "<Placemark>"
        "<name>Monongahela National Forest</name>"
        "<description><![CDATA[Huttonsville, WV 26273<br>]]></description>"
        "<styleUrl>#style3</styleUrl>"
        "<Point>"
          "<coordinates>-79.829674,38.627785,0.000000</coordinates>"
        "</Point>"
      "</Placemark>"
      "<Placemark>"
        "<name>From: Минск, Минская область, Беларусь</name>"
        "<description><![CDATA[]]></description>"
        "<styleUrl>#style5</styleUrl>"
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
    bm = cat.GetBookmark(1);
    TEST_EQUAL(bm->GetName(), "Monongahela National Forest", ());

    bm = cat.GetBookmark(2);
    m2::PointD const org = bm->GetOrg();
    TEST_ALMOST_EQUAL(MercatorBounds::XToLon(org.x), 27.566765, ());
    TEST_ALMOST_EQUAL(MercatorBounds::YToLat(org.y), 53.900047, ());
  }
}

UNIT_TEST(Bookmarks_ImportKML)
{
  BookmarkCategory cat("Default");
  cat.LoadFromKML(new MemReader(kmlString, strlen(kmlString)));

  CheckBookmarks(cat);
}

UNIT_TEST(Bookmarks_ExportKML)
{
  BookmarkCategory cat("Default");
  cat.LoadFromKML(new MemReader(kmlString, strlen(kmlString)));

  CheckBookmarks(cat);

  {
    ofstream of("Bookmarks.kml");
    cat.SaveToKML(of);
  }

  cat.ClearBookmarks();

  TEST_EQUAL(cat.GetBookmarksCount(), 0, ());

  cat.LoadFromKML(new FileReader("Bookmarks.kml"));

  CheckBookmarks(cat);
}

UNIT_TEST(Bookmarks_Getting)
{
  Framework fm;
  fm.OnSize(800, 400);
  fm.ShowRect(m2::RectD(0, 0, 80, 40));

  m2::PointD const pixC = fm.GtoP(m2::PointD(40, 20));

  // This is not correct because Framework::OnSize doesn't work until SetRenderPolicy is called.
  //TEST(m2::AlmostEqual(m2::PointD(400, 200), pixC), (pixC));

  fm.AddBookmark("cat1", Bookmark(m2::PointD(38, 20), "1", "red"));
  fm.AddBookmark("cat2", Bookmark(m2::PointD(41, 20), "2", "red"));
  fm.AddBookmark("cat3", Bookmark(m2::PointD(41, 40), "3", "red"));

  Bookmark const * bm = fm.GetBookmark(pixC, 1.0);
  TEST(bm != 0, ());
  TEST_EQUAL(bm->GetName(), "2", ());

  TEST(fm.GetBookmark(m2::PointD(0, 0)) == 0, ());
  TEST(fm.GetBookmark(m2::PointD(800, 400)) == 0, ());
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
