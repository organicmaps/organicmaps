#include "testing/testing.hpp"

#include "kml/serdes_gpx.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/mercator.hpp"

char const * kTextGpx =
R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.0">
 <wpt lat="42.81025" lon="-1.65727">
  <time>2022-09-05T08:39:39.3700Z</time>
  <name>Waypoint 1</name>
 </wpt>
)";

auto const kDefaultLang = StringUtf8Multilang::kDefaultCode;

UNIT_TEST(Gpx_Test)
{

  kml::FileData dataFromText;
  try
  {
    kml::DeserializerGpx des(dataFromText);
    MemReader reader(kTextGpx, strlen(kTextGpx));
    des.Deserialize(reader);
  }
  catch (kml::DeserializerGpx::DeserializeException const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

    kml::FileData data;
    kml::BookmarkData bookmarkData;
    bookmarkData.m_name[kDefaultLang] = "Waypoint 1";
    bookmarkData.m_point = mercator::FromLatLon(42.81025, -1.65727);
    data.m_bookmarksData.emplace_back(std::move(bookmarkData));

  TEST_EQUAL(dataFromText, data, ());
}

