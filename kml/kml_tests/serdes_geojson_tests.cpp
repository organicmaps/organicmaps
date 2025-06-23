#include "testing/testing.hpp"

#include "kml/kml_tests/tests_data.hpp"

//#include "kml/serdes.hpp"
//#include "kml/serdes_binary.hpp"
#include "kml/serdes_geojson.hpp"

#include "map/bookmark_helpers.hpp"

#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/hex.hpp"
#include "coding/reader.hpp"
#include "coding/string_utf8_multilang.hpp"
#include "coding/writer.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <cstring>
#include <functional>
#include <sstream>
#include <vector>

namespace
{
// This function can be used to generate textual representation of vector<uint8_t> like you see above.
/*std::string FormatBytesFromBuffer(std::vector<uint8_t> const & buffer)
{
  std::stringstream ss;
  for (size_t i = 1; i <= buffer.size(); i++)
  {
    ss << "0x" << NumToHex(buffer[i - 1]) << ", ";
    if (i % 16 == 0)
      ss << "\n";
  }
  return ss.str();
}

auto const kDefaultLang = StringUtf8Multilang::kDefaultCode;
auto const kEnLang = StringUtf8Multilang::kEnglishCode;
auto const kRuLang = static_cast<int8_t>(8);
*/

UNIT_TEST(GeoJson_parse)
{
  std::string_view constexpr data = R"({
    "type": "FeatureCollection",
    "features": [
      {
        "type": "Feature",
        "properties": {},
        "geometry": {
          "coordinates": [
            [
              14.949382505528291,
              8.16007148457335
            ],
            [
              26.888888114204264,
              9.708105796659268
            ],
            [
              37.54707497642465,
              6.884595662842159
            ]
          ],
          "type": "LineString"
        }
      },
      {
        "type": "Feature",
        "properties": {},
        "geometry": {
          "coordinates": [
            31.02177966625902,
            29.8310316130992
          ],
          "type": "Point"
        }
      }
    ]
  })";

  kml::FileData fData;
  TEST_NO_THROW(
  {
    kml::geojson::GeojsonParser(fData).Parse(MemReader(data));
  }, ());

  TEST_EQUAL(fData.m_tracksData.size(), 1, ());
  auto const & geom = fData.m_tracksData[0].m_geometry;
  auto const & lines = geom.m_lines;
  auto const & timestamps = geom.m_timestamps;
  TEST_EQUAL(lines.size(), 2, ());
  TEST_EQUAL(lines[0].size(), 7, ());
  TEST_EQUAL(lines[1].size(), 6, ());
  TEST(geom.HasTimestamps(), ());
  TEST(geom.HasTimestampsFor(0), ());
  TEST(geom.HasTimestampsFor(1), ());
  TEST_EQUAL(timestamps.size(), 2, ());
  TEST_EQUAL(timestamps[0].size(), 7, ());
  TEST_EQUAL(timestamps[1].size(), 6, ());
}
}
