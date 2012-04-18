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

  void CheckBookmarks(Framework const & fm)
  {
    TEST_EQUAL(fm.BookmarksCount(), 3, ());

    Bookmark bm;
    fm.GetBookmark(0, bm);
    TEST_EQUAL(bm.GetName(), "Nebraska", ());
    fm.GetBookmark(1, bm);
    TEST_EQUAL(bm.GetName(), "Monongahela National Forest", ());

    fm.GetBookmark(2, bm);
    m2::PointD const org = bm.GetOrg();
    TEST_ALMOST_EQUAL(MercatorBounds::XToLon(org.x), 27.566765, ());
    TEST_ALMOST_EQUAL(MercatorBounds::YToLat(org.y), 53.900047, ());
  }
}

UNIT_TEST(Bookmarks_ImportKML)
{
  Framework fm;
  fm.LoadFromKML(new MemReader(kmlString, strlen(kmlString)));

  CheckBookmarks(fm);
}

UNIT_TEST(Bookmarks_ExportKML)
{
  Framework fm;
  fm.LoadFromKML(new MemReader(kmlString, strlen(kmlString)));

  CheckBookmarks(fm);

  {
    ofstream of("Bookmarks.kml");
    fm.SaveToKML(of);
  }

  fm.ClearBookmarks();

  TEST_EQUAL(fm.BookmarksCount(), 0, ());

  fm.LoadFromKML(new FileReader("Bookmarks.kml"));

  CheckBookmarks(fm);
}

UNIT_TEST(Bookmarks_Getting)
{
  Framework fm;
  fm.OnSize(800, 400);
  fm.ShowRect(m2::RectD(0, 0, 80, 40));

  m2::PointD const pixC = fm.GtoP(m2::PointD(40, 20));

  // This is not correct because Framework::OnSize doesn't work until SetRenderPolicy is called.
  //TEST(m2::AlmostEqual(m2::PointD(400, 200), pixC), (pixC));

  fm.AddBookmark(m2::PointD(38, 20), "1");
  fm.AddBookmark(m2::PointD(41, 20), "2");
  fm.AddBookmark(m2::PointD(41, 40), "3");

  Bookmark bm;
  TEST_EQUAL(fm.GetBookmark(pixC, bm), 1, ());
  TEST_EQUAL(bm.GetName(), "2", ());
}
