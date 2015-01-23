#include "testing/testing.hpp"

#include "indexer/data_header.hpp"

#include "map/framework.hpp"

#include "search/result.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "graphics/color.hpp"

#include "coding/internal/file_data.hpp"

#include "std/fstream.hpp"
#include "std/unique_ptr.hpp"

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
    TEST_EQUAL(cat.GetUserMarkCount(), 4, ());

    Bookmark const * bm = static_cast<Bookmark const *>(cat.GetUserMark(3));
    TEST_EQUAL(bm->GetName(), "Nebraska", ());
    TEST_EQUAL(bm->GetType(), "placemark-red", ());
    TEST_EQUAL(bm->GetDescription(), "", ());
    TEST_EQUAL(bm->GetTimeStamp(), my::INVALID_TIME_STAMP, ());

    bm = static_cast<Bookmark const *>(cat.GetUserMark(2));
    TEST_EQUAL(bm->GetName(), "Monongahela National Forest", ());
    TEST_EQUAL(bm->GetType(), "placemark-pink", ());
    TEST_EQUAL(bm->GetDescription(), "Huttonsville, WV 26273<br>", ());
    TEST_EQUAL(bm->GetTimeStamp(), 524214643, ());

    bm = static_cast<Bookmark const *>(cat.GetUserMark(1));
    m2::PointD org = bm->GetOrg();
    TEST_ALMOST_EQUAL_ULPS(MercatorBounds::XToLon(org.x), 27.566765, ());
    TEST_ALMOST_EQUAL_ULPS(MercatorBounds::YToLat(org.y), 53.900047, ());
    TEST_EQUAL(bm->GetName(), "From: Минск, Минская область, Беларусь", ());
    TEST_EQUAL(bm->GetType(), "placemark-blue", ());
    TEST_EQUAL(bm->GetDescription(), "", ());
    TEST_EQUAL(bm->GetTimeStamp(), 888888888, ());

    bm = static_cast<Bookmark const *>(cat.GetUserMark(0));
    org = bm->GetOrg();
    TEST_ALMOST_EQUAL_ULPS(MercatorBounds::XToLon(org.x), 27.551532, ());
    TEST_ALMOST_EQUAL_ULPS(MercatorBounds::YToLat(org.y), 53.89306, ());
    TEST_EQUAL(bm->GetName(), "<MWM & Sons>", ());
    TEST_EQUAL(bm->GetDescription(), "Amps & <brackets>", ());
    TEST_EQUAL(bm->GetTimeStamp(), my::INVALID_TIME_STAMP, ());
  }
}

UNIT_TEST(Bookmarks_ImportKML)
{
  Framework framework;
  BookmarkCategory cat("Default", framework);
  TEST(cat.LoadFromKML(new MemReader(kmlString, strlen(kmlString))), ());

  CheckBookmarks(cat);

  // Name should be overridden from file
  TEST_EQUAL(cat.GetName(), "MapName", ());
  TEST_EQUAL(cat.IsVisible(), false, ());
}

UNIT_TEST(Bookmarks_ExportKML)
{
  char const * BOOKMARKS_FILE_NAME = "UnitTestBookmarks.kml";

  Framework framework;
  BookmarkCategory cat("Default", framework);
  BookmarkCategory::Guard guard(cat);
  TEST(cat.LoadFromKML(new MemReader(kmlString, strlen(kmlString))), ());
  CheckBookmarks(cat);

  TEST_EQUAL(cat.IsVisible(), false, ());
  // Change visibility
  guard.m_controller.SetIsVisible(true);
  TEST_EQUAL(cat.IsVisible(), true, ());

  {
    ofstream of(BOOKMARKS_FILE_NAME);
    cat.SaveToKML(of);
  }

  guard.m_controller.Clear();
  TEST_EQUAL(guard.m_controller.GetUserMarkCount(), 0, ());

  TEST(cat.LoadFromKML(new FileReader(BOOKMARKS_FILE_NAME)), ());
  CheckBookmarks(cat);
  TEST_EQUAL(cat.IsVisible(), true, ());

  unique_ptr<BookmarkCategory> cat2(BookmarkCategory::CreateFromKMLFile(BOOKMARKS_FILE_NAME, framework));
  CheckBookmarks(*cat2);

  TEST(cat2->SaveToKMLFile(), ());
  // old file should be deleted if we save bookmarks with new category name
  uint64_t dummy;
  TEST(!my::GetFileSize(BOOKMARKS_FILE_NAME, dummy), ());

  // MapName is the <name> tag in test kml data.
  string const catFileName = GetPlatform().SettingsDir() + "MapName.kml";
  cat2.reset(BookmarkCategory::CreateFromKMLFile(catFileName, framework));
  CheckBookmarks(*cat2);
  TEST(my::DeleteFileX(catFileName), ());
}

namespace
{
  template <size_t N> void DeleteCategoryFiles(char const * (&arrFiles)[N])
  {
    string const path = GetPlatform().SettingsDir();
    for (size_t i = 0; i < N; ++i)
      FileWriter::DeleteFileX(path + arrFiles[i] + BOOKMARKS_FILE_EXTENSION);
  }

  UserMark const * GetMark(Framework & fm, m2::PointD const & pt)
  {
    m2::AnyRectD rect;
    fm.GetNavigator().GetTouchRect(fm.GtoP(pt), 20, rect);

    return fm.GetBookmarkManager().FindNearestUserMark(rect);
  }

  Bookmark const * GetBookmark(Framework & fm, m2::PointD const & pt)
  {
    UserMark const * mark = GetMark(fm, pt);
    ASSERT(mark != NULL, ());
    ASSERT(mark->GetContainer() != NULL, ());
    ASSERT(mark->GetContainer()->GetType() == UserMarkType::BOOKMARK_MARK, ());
    return static_cast<Bookmark const *>(mark);
  }

  Bookmark const * GetBookmarkPxPoint(Framework & fm, m2::PointD const & pt)
  {
    return GetBookmark(fm, fm.PtoG(pt));
  }

  BookmarkCategory const * GetCategory(Bookmark const * bm)
  {
    ASSERT(bm->GetContainer() != NULL, ());
    ASSERT(bm->GetContainer()->GetType() == UserMarkType::BOOKMARK_MARK, ());
    return static_cast<BookmarkCategory const *>(bm->GetContainer());
  }

  bool IsValidBookmark(Framework & fm, m2::PointD const & pt)
  {
    UserMark const * mark = GetMark(fm, pt);
    if (mark == NULL)
      return false;

    if (mark->GetContainer()->GetType() != UserMarkType::BOOKMARK_MARK)
      return false;

    return true;
  }
}

UNIT_TEST(Bookmarks_Timestamp)
{
  Framework fm;
  m2::PointD const orgPoint(10, 10);

  char const * arrCat[] = { "cat", "cat1" };

  BookmarkData b1("name", "type");
  TEST_EQUAL(fm.AddCategory(arrCat[0]), 0, ());
  TEST_EQUAL(fm.AddBookmark(0, orgPoint, b1), 0, ());

  Bookmark const * pBm = GetBookmark(fm, orgPoint);
  TEST_NOT_EQUAL(pBm->GetTimeStamp(), my::INVALID_TIME_STAMP, ());

  BookmarkData b3("newName", "newType");
  TEST_EQUAL(fm.AddBookmark(0, orgPoint, b3), 0, ());

  fm.AddCategory(arrCat[1]);
  TEST_EQUAL(fm.AddBookmark(1, orgPoint, b3), 0, ());

  // Check bookmarks order here. First added should be in the bottom of the list.
  TEST_EQUAL(fm.GetBmCategory(0)->GetBookmark(1), pBm, ());

  Bookmark const * bm01 = static_cast<Bookmark const *>(fm.GetBmCategory(0)->GetUserMark(1));

  TEST_EQUAL(bm01->GetName(), "name", ());
  TEST_EQUAL(bm01->GetType(), "type", ());

  Bookmark const * bm00 = static_cast<Bookmark const *>(fm.GetBmCategory(0)->GetUserMark(0));

  TEST_EQUAL(bm00->GetName(), "newName", ());
  TEST_EQUAL(bm00->GetType(), "newType", ());

  Bookmark const * bm10 = static_cast<Bookmark const *>(fm.GetBmCategory(1)->GetUserMark(0));

  TEST_EQUAL(bm10->GetName(), "newName", ());
  TEST_EQUAL(bm10->GetType(), "newType", ());

  TEST_EQUAL(fm.GetBmCategory(0)->GetUserMarkCount(), 2, ());
  TEST_EQUAL(fm.GetBmCategory(1)->GetUserMarkCount(), 1, ());

  DeleteCategoryFiles(arrCat);
}

UNIT_TEST(Bookmarks_Getting)
{
  Framework fm;
  fm.OnSize(800, 400);
  fm.ShowRect(m2::RectD(0, 0, 80, 40));

  // This is not correct because Framework::OnSize doesn't work until SetRenderPolicy is called.
  //TEST(m2::AlmostEqualULPs(m2::PointD(400, 200), pixC), (pixC));

  char const * arrCat[] = { "cat1", "cat2", "cat3" };
  for (int i = 0; i < 3; ++i)
    fm.AddCategory(arrCat[i]);

  BookmarkData bm("1", "placemark-red");
  fm.AddBookmark(0, m2::PointD(38, 20), bm);
  BookmarkCategory const * c1 = fm.GetBmCategory(0);
  bm = BookmarkData("2", "placemark-red");
  fm.AddBookmark(1, m2::PointD(41, 20), bm);
  BookmarkCategory const * c2 = fm.GetBmCategory(1);
  bm = BookmarkData("3", "placemark-red");
  fm.AddBookmark(2, m2::PointD(41, 40), bm);
  BookmarkCategory const * c3 = fm.GetBmCategory(2);


  TEST_NOT_EQUAL(c1, c2, ());
  TEST_NOT_EQUAL(c2, c3, ());
  TEST_NOT_EQUAL(c1, c3, ());

  (void)fm.GetBmCategory(4);
  TEST_EQUAL(fm.GetBmCategoriesCount(), 3, ());

  Bookmark const * mark = GetBookmark(fm, m2::PointD(40, 20));
  BookmarkCategory const * cat = GetCategory(mark);

  TEST_EQUAL(cat->GetName(), arrCat[1], ());

  TEST(!IsValidBookmark(fm, m2::PointD(0, 0)), ());
  TEST(!IsValidBookmark(fm, m2::PointD(800, 400)), ());

  TEST(IsValidBookmark(fm, m2::PointD(41, 40)), ());
  mark = GetBookmark(fm, m2::PointD(41, 40));
  cat = GetCategory(mark);
  TEST_EQUAL(cat->GetName(), arrCat[2], ());

  bm = BookmarkData("4", "placemark-blue");
  fm.AddBookmark(2, m2::PointD(41, 40), bm);
  BookmarkCategory const * c33 = fm.GetBmCategory(2);

  TEST_EQUAL(c33, c3, ());

  mark = GetBookmark(fm, m2::PointD(41, 40));
  cat = GetCategory(mark);

  // Should find last added valid result, there two results with the
  // same coordinates 3 and 4, but 4 was added later.
  TEST_EQUAL(mark->GetName(), "4", ());
  TEST_EQUAL(mark->GetType(), "placemark-blue", ());

  TEST_EQUAL(cat->GetUserMarkCount(), 2, ());

  BookmarkCategory::Guard guard(*fm.GetBmCategory(2));
  guard.m_controller.DeleteUserMark(0);
  TEST_EQUAL(cat->GetUserMarkCount(), 1, ());

  DeleteCategoryFiles(arrCat);
}

namespace
{
  struct POIInfo
  {
    char const * m_name;
    char const * m_street;
    char const * m_house;
    char const * m_type;
  };

  void CheckPlace(Framework const & fm, double lat, double lon, POIInfo const & poi)
  {
    search::AddressInfo info;
    fm.GetAddressInfoForGlobalPoint(MercatorBounds::FromLatLon(lat, lon), info);

    TEST_EQUAL(info.m_name, poi.m_name, ());
    TEST_EQUAL(info.m_street, poi.m_street, ());
    TEST_EQUAL(info.m_house, poi.m_house, ());
    TEST_EQUAL(info.m_types.size(), 1, ());
    TEST_EQUAL(info.GetBestType(), poi.m_type, ());
  }
}

UNIT_TEST(Bookmarks_AddressInfo)
{
  // Maps added in constructor (we need minsk-pass.mwm only)
  Framework fm;
  fm.DeregisterAllMaps();
  fm.RegisterMap(platform::LocalCountryFile::MakeForTesting("minsk-pass"));
  fm.OnSize(800, 600);

  // assume that developers have English or Russian system language :)
  string const lang = languages::GetCurrentNorm();
  LOG(LINFO, ("Current language =", lang));

  // default name (street in russian, category in english).
  size_t index = 0;
  if (lang == "ru")
    index = 1;
  if (lang == "en")
    index = 2;

  POIInfo poi1[] = {
    { "Планета Pizza", "улица Карла Маркса", "10", "cafe" },
    { "Планета Pizza", "улица Карла Маркса", "10", "кафе" },
    { "Планета Pizza", "vulica Karla Marksa", "10", "cafe" }
  };
  CheckPlace(fm, 53.8964918, 27.555559, poi1[index]);

  POIInfo poi2[] = {
    { "Нц Шашек И Шахмат", "улица Карла Маркса", "10", "hotel" },
    { "Нц Шашек И Шахмат", "улица Карла Маркса", "10", "гостиница" },
    { "Нц Шашек И Шахмат", "vulica Karla Marksa", "10", "hotel" }
  };
  CheckPlace(fm, 53.8964365, 27.5554007, poi2[index]);
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
  string const FILENAME = BASE + BOOKMARKS_FILE_EXTENSION;

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

  char const * arrCat[] = { "cat1", "cat2" };
  for (int i = 0; i < 2; ++i)
    fm.AddCategory(arrCat[i]);

  m2::PointD const globalPoint = m2::PointD(40, 20);
  m2::PointD const pixelPoint = fm.GtoP(globalPoint);

  BookmarkData bm("name", "placemark-red");
  fm.AddBookmark(0, globalPoint, bm);
  BookmarkCategory const * c1 = fm.GetBmCategory(0);
  Bookmark const * mark = GetBookmarkPxPoint(fm, pixelPoint);
  BookmarkCategory const * cat = GetCategory(mark);
  TEST_EQUAL(cat->GetName(), arrCat[0], ());

  bm = BookmarkData("name2", "placemark-blue");
  fm.AddBookmark(0, globalPoint, bm);
  BookmarkCategory const * c11 = fm.GetBmCategory(0);
  TEST_EQUAL(c1, c11, ());
  mark = GetBookmarkPxPoint(fm, pixelPoint);
  cat = GetCategory(mark);
  TEST_EQUAL(cat->GetName(), arrCat[0], ());
  TEST_EQUAL(mark->GetName(), "name2", ());
  TEST_EQUAL(mark->GetType(), "placemark-blue", ());

  // Edit name, type and category of bookmark
  bm = BookmarkData("name3", "placemark-green");
  fm.AddBookmark(1, globalPoint, bm);
  BookmarkCategory const * c2 = fm.GetBmCategory(1);
  TEST_NOT_EQUAL(c1, c2, ());
  TEST_EQUAL(fm.GetBmCategoriesCount(), 2, ());
  mark = GetBookmarkPxPoint(fm, pixelPoint);
  cat = GetCategory(mark);
  TEST_EQUAL(cat->GetName(), arrCat[0], ());
  TEST_EQUAL(fm.GetBmCategory(0)->GetUserMarkCount(), 2,
             ("Bookmark wasn't moved from one category to another"));
  TEST_EQUAL(mark->GetName(), "name2", ());
  TEST_EQUAL(mark->GetType(), "placemark-blue", ());

  DeleteCategoryFiles(arrCat);
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
  Framework framework;
  BookmarkCategory cat("Default", framework);
  TEST(cat.LoadFromKML(new MemReader(kmlString2, strlen(kmlString2))), ());

  TEST_EQUAL(cat.GetUserMarkCount(), 1, ());
}

UNIT_TEST(BookmarkCategory_EmptyName)
{
  Framework framework;
  unique_ptr<BookmarkCategory> pCat(new BookmarkCategory("", framework));
  {
    BookmarkCategory::Guard guard(*pCat);
    static_cast<Bookmark *>(guard.m_controller.CreateUserMark(m2::PointD(0, 0)))->SetData(BookmarkData("", "placemark-red"));
  }
  TEST(pCat->SaveToKMLFile(), ());

  pCat->SetName("xxx");
  TEST(pCat->SaveToKMLFile(), ());

  char const * arrFiles[] = { "Bookmarks", "xxx" };
  DeleteCategoryFiles(arrFiles);
}

namespace
{
// Do check for "bad" names without CDATA markers.
char const * kmlString3 =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<kml xmlns=\"http://earth.google.com/kml/2.1\">"
    "<Document>"
     "<name>3663 and M <![CDATA[&]]> J Seafood Branches</name>"
     "<visibility>1</visibility>"
      "<Placemark>"
       "<name>![X1]{X2}(X3)</name>"
       "<Point>"
        "<coordinates>50, 50</coordinates>"
       "</Point>"
      "</Placemark>"
   "</Document>"
   "</kml>";

  bool EqualBookmarks(Bookmark const & b1, Bookmark const & b2)
  {
    if (b1.GetName() != b2.GetName())
      return false;
    if (b1.GetDescription() != b2.GetDescription())
      return false;
    if (b1.GetType() != b2.GetType())
      return false;
    if (!m2::AlmostEqualULPs(b1.GetOrg(), b2.GetOrg()))
      return false;
    if (!my::AlmostEqualULPs(b1.GetScale(), b2.GetScale()))
      return false;

    // do not check timestamp
    return true;
  }
}

UNIT_TEST(Bookmarks_SpecialXMLNames)
{
  Framework framework;
  BookmarkCategory cat1("", framework);
  TEST(cat1.LoadFromKML(new MemReader(kmlString3, strlen(kmlString3))), ());

  TEST_EQUAL(cat1.GetUserMarkCount(), 1, ());
  TEST(cat1.SaveToKMLFile(), ());

  unique_ptr<BookmarkCategory> const cat2(BookmarkCategory::CreateFromKMLFile(cat1.GetFileName(), framework));
  TEST(cat2.get(), ());
  TEST_EQUAL(cat2->GetUserMarkCount(), 1, ());

  TEST_EQUAL(cat1.GetName(), "3663 and M & J Seafood Branches", ());
  TEST_EQUAL(cat1.GetName(), cat2->GetName(), ());
  TEST_EQUAL(cat1.GetFileName(), cat2->GetFileName(), ());
  Bookmark const * bm1 = static_cast<Bookmark const *>(cat1.GetUserMark(0));
  Bookmark const * bm2 = static_cast<Bookmark const *>(cat2->GetUserMark(0));
  TEST(EqualBookmarks(*bm1, *bm2), ());
  TEST_EQUAL(bm1->GetName(), "![X1]{X2}(X3)", ());

  TEST(my::DeleteFileX(cat1.GetFileName()), ());
}

UNIT_TEST(TrackParsingTest_1)
{
  Framework framework;
  string const kmlFile = GetPlatform().TestsDataPathForFile("kml-with-track-kml.test");
  BookmarkCategory * cat = BookmarkCategory::CreateFromKMLFile(kmlFile, framework);
  TEST(cat, ("Category can't be created"));

  TEST_EQUAL(cat->GetTracksCount(), 4, ());

  string names[4] = { "Option1", "Pakkred1", "Pakkred2", "Pakkred3"};
  dp::Color col[4] = {dp::Color(230, 0, 0, 255),
                      dp::Color(171, 230, 0, 255),
                      dp::Color(0, 230, 117, 255),
                      dp::Color(0, 59, 230, 255)};
  double length[4] = {3525.46839061, 27174.11393166, 27046.0456586, 23967.35765800};

  for (size_t i = 0; i < ARRAY_SIZE(names); ++i)
  {
    Track const * track = cat->GetTrack(i);
    TEST_EQUAL(names[i], track->GetName(), ());
    TEST(fabs(track->GetLengthMeters() - length[i]) < 1.0E-6, (track->GetLengthMeters(), length[i]));
    TEST_EQUAL(col[i], track->GetMainColor(), ());
  }
}

UNIT_TEST(TrackParsingTest_2)
{
  Framework framework;
  string const kmlFile = GetPlatform().TestsDataPathForFile("kml-with-track-from-google-earth.test");
  BookmarkCategory * cat = BookmarkCategory::CreateFromKMLFile(kmlFile, framework);
  TEST(cat, ("Category can't be created"));

  TEST_EQUAL(cat->GetTracksCount(), 1, ());
  Track const * track = cat->GetTrack(0);
  TEST_EQUAL(track->GetName(), "XY", ());
  TEST_EQUAL(track->GetMainColor(), dp::Color(57, 255, 32, 255), ());
}

