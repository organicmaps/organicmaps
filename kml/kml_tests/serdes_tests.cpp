#include "testing/testing.hpp"

#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/hex.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/scope_guard.hpp"
#include "coding/multilang_utf8_string.hpp"

#include <chrono>
#include <cstring>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>

namespace
{
char const * kTextKml =
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

std::vector<uint8_t> const kBinKml = {
  0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x24, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x00,
  0x01, 0x01, 0x00, 0x02, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE2,
  0xAB, 0xEB, 0x96, 0xE2, 0xFE, 0x83, 0xD4, 0x11, 0x00, 0x02, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF3,
  0xC2, 0xFB, 0xF9, 0x01, 0x9E, 0xD0, 0xC7, 0x95, 0x9B, 0x9D, 0xBD, 0xAB, 0x12, 0x00, 0x02, 0x01,
  0x00, 0x04, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB8, 0xBC, 0xED, 0xA7, 0x03, 0xEA, 0xA4, 0xD0, 0x9C, 0xD9,
  0xA7, 0xBE, 0x9A, 0x19, 0x00, 0x02, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE4, 0xAF, 0xA7, 0xC1,
  0xEB, 0xA6, 0xBE, 0x9A, 0x19, 0x00, 0x02, 0x01, 0x00, 0x07, 0x01, 0x00, 0x08, 0x00, 0x54, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0E, 0x08, 0x1B, 0x1A, 0x41, 0x0C, 0x11, 0x25,
  0x3F, 0x00, 0x02, 0x00, 0x01, 0x08, 0x03, 0x02, 0x06, 0x01, 0x03, 0x06, 0x40, 0x05, 0x07, 0x8D,
  0x01, 0x06, 0x05, 0x02, 0x07, 0x07, 0x71, 0x09, 0x05, 0x05, 0x0A, 0x07, 0x12, 0x0B, 0x06, 0x5E,
  0x0D, 0x06, 0xBA, 0x01, 0x0E, 0x05, 0x04, 0x0F, 0x05, 0x75, 0x11, 0x05, 0x0D, 0x12, 0x06, 0x09,
  0x13, 0x07, 0x4E, 0x15, 0x06, 0x91, 0x01, 0x16, 0x06, 0x0B, 0x17, 0x06, 0x73, 0x19, 0x05, 0x06,
  0x1A, 0x08, 0x1C, 0x1B, 0x07, 0x63, 0x1D, 0x06, 0xC0, 0x01, 0x1E, 0x05, 0x07, 0x1F, 0x06, 0x77,
  0x21, 0x06, 0x0A, 0x22, 0x07, 0x08, 0x23, 0x07, 0x49, 0x25, 0x07, 0x90, 0x01, 0x27, 0x06, 0x72,
  0x2A, 0x08, 0x14, 0x2B, 0x06, 0x62, 0x2D, 0x06, 0xBD, 0x01, 0x32, 0x06, 0x0C, 0x33, 0x06, 0x57,
  0x35, 0x07, 0xA1, 0x01, 0x36, 0x06, 0x0F, 0x37, 0x07, 0x76, 0x3A, 0x08, 0x31, 0x3B, 0x06, 0x68,
  0x3D, 0x06, 0xD1, 0x01, 0x3F, 0x06, 0x79, 0x41, 0x07, 0xB8, 0x01, 0x45, 0x07, 0x8F, 0x01, 0x47,
  0x07, 0x74, 0x4A, 0x07, 0x13, 0x53, 0x07, 0x59, 0x5A, 0x07, 0x29, 0x5B, 0x07, 0x6D, 0x62, 0x08,
  0x10, 0x63, 0x07, 0x4B, 0x65, 0x07, 0x96, 0x01, 0x6A, 0x08, 0x18, 0x75, 0x07, 0xB7, 0x01, 0x77,
  0x07, 0x8A, 0x01, 0x7A, 0x08, 0x3F, 0x81, 0x01, 0x08, 0x0E, 0x9A, 0x01, 0x08, 0x25, 0xAA, 0x01,
  0x08, 0x17, 0xBA, 0x01, 0x08, 0x3E, 0xE2, 0x01, 0x08, 0x11, 0xEA, 0x01, 0x08, 0x19, 0xFA, 0x01,
  0x08, 0x41, 0xB0, 0x01, 0xD7, 0x64, 0xD6, 0x6E, 0x37, 0x46, 0x07, 0x3D, 0xA1, 0xA8, 0x65, 0x2F,
  0x34, 0x01, 0x83, 0xBE, 0x2E, 0x87, 0x3C, 0x64, 0x71, 0x6E, 0xA3, 0xE7, 0xCC, 0xB7, 0x8E, 0x6D,
  0xA7, 0xE2, 0xFD, 0x13, 0xBC, 0x9A, 0x51, 0x7D, 0xF2, 0xE9, 0x17, 0x8F, 0x33, 0x91, 0x28, 0xDB,
  0xA1, 0x22, 0x5F, 0xB3, 0x14, 0x65, 0xE9, 0x57, 0x92, 0xAC, 0x33, 0xC5, 0xF8, 0x3E, 0x37, 0x35,
  0x00, 0x88, 0x1C, 0x96, 0x62, 0x97, 0x17, 0x09, 0xA5, 0x3F, 0x42, 0x18, 0x98, 0xD5, 0x45, 0x96,
  0x1D, 0x0A, 0x46, 0x2C, 0xB9, 0x94, 0xFE, 0x35, 0xF7, 0x00, 0x00, 0xF4, 0x00, 0x00, 0x00, 0x00,
  0x1A, 0xD0, 0xA2, 0xD9, 0x95, 0x6B, 0xDC, 0x69, 0xEA, 0x2D, 0x52, 0xB0, 0x43, 0xA8, 0x7C, 0xA9,
  0x32, 0x0E, 0x01, 0x00, 0x09,
};

// This function can be used to generate textual representation of vector<uint8_t> like you see above.
std::string FormatBytesFromBuffer(std::vector<uint8_t> const & buffer)
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
auto const kRuLang = static_cast<int8_t>(8);

kml::CategoryData GenerateCategoryData()
{
  kml::CategoryData categoryData;
  categoryData.m_name[kDefaultLang] = "Test category";
  categoryData.m_name[kRuLang] = "Тестовая категория";
  categoryData.m_description[kDefaultLang] = "Test description";
  categoryData.m_description[kRuLang] = "Тестовое описание";
  categoryData.m_visible = true;
  categoryData.m_authorName = "Maps.Me";
  categoryData.m_authorId = "12345";
  categoryData.m_lastModified = std::chrono::system_clock::from_time_t(1000);
  categoryData.m_accessRules = kml::AccessRules::Public;
  categoryData.m_tags = {"mountains", "ski", "snowboard"};
  categoryData.m_toponyms = {"Georgia", "Gudauri"};
  categoryData.m_languageCodes = {1, 2, 8};

  kml::BookmarkData bookmarkData;
  bookmarkData.m_id = 10;
  bookmarkData.m_name[kDefaultLang] = "Test bookmark";
  bookmarkData.m_name[kRuLang] = "Тестовая метка";
  bookmarkData.m_description[kDefaultLang] = "Test bookmark description";
  bookmarkData.m_description[kRuLang] = "Тестовое описание метки";
  bookmarkData.m_types = {8, 13, 34, 565};
  bookmarkData.m_color = {kml::PredefinedColor::Blue, 0};
  bookmarkData.m_icon = kml::BookmarkIcon::None;
  bookmarkData.m_viewportScale = 15;
  bookmarkData.m_timestamp = std::chrono::system_clock::from_time_t(800);
  bookmarkData.m_point = m2::PointD(45.9242, 56.8679);
  bookmarkData.m_boundTracks = {0};
  categoryData.m_bookmarksData.emplace_back(std::move(bookmarkData));

  kml::TrackData trackData;
  trackData.m_id = 121;
  trackData.m_localId = 0;
  trackData.m_name[kDefaultLang] = "Test track";
  trackData.m_name[kRuLang] = "Тестовый трек";
  trackData.m_description[kDefaultLang] = "Test track description";
  trackData.m_description[kRuLang] = "Тестовое описание трека";
  trackData.m_layers = {{6.0, {kml::PredefinedColor::None, 0xff0000ff}},
                        {7.0, {kml::PredefinedColor::None, 0x00ff00ff}}};
  trackData.m_timestamp = std::chrono::system_clock::from_time_t(900);
  trackData.m_points = {m2::PointD(45.9242, 56.8679), m2::PointD(45.2244, 56.2786),
                        m2::PointD(45.1964, 56.9832)};
  categoryData.m_tracksData.emplace_back(std::move(trackData));

  return categoryData;
}

char const * kGeneratedKml =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
  "<Document>\n"
  "  <Style id=\"placemark-red\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://mapswith.me/placemarks/placemark-red.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-blue\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://mapswith.me/placemarks/placemark-blue.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-purple\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://mapswith.me/placemarks/placemark-purple.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-yellow\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://mapswith.me/placemarks/placemark-yellow.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-pink\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://mapswith.me/placemarks/placemark-pink.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-brown\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://mapswith.me/placemarks/placemark-brown.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-green\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://mapswith.me/placemarks/placemark-green.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-orange\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://mapswith.me/placemarks/placemark-orange.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <name>Test category</name>\n"
  "  <description>Test description</description>\n"
  "  <visibility>1</visibility>\n"
  "  <ExtendedData xmlns:mwm=\"https://maps.me\">\n"
  "    <mwm:name>\n"
  "      <mwm:lang code=\"ru\">Тестовая категория</mwm:lang>\n"
  "    </mwm:name>\n"
  "    <mwm:description>\n"
  "      <mwm:lang code=\"ru\">Тестовое описание</mwm:lang>\n"
  "    </mwm:description>\n"
  "    <mwm:author id=\"12345\">Maps.Me</mwm:author>\n"
  "    <mwm:lastModified>1970-01-01T00:16:40Z</mwm:lastModified>\n"
  "    <mwm:accessRules>Public</mwm:accessRules>\n"
  "    <mwm:tags>\n"
  "      <mwm:value>mountains</mwm:value>\n"
  "      <mwm:value>ski</mwm:value>\n"
  "      <mwm:value>snowboard</mwm:value>\n"
  "    </mwm:tags>\n"
  "    <mwm:toponyms>\n"
  "      <mwm:value>Georgia</mwm:value>\n"
  "      <mwm:value>Gudauri</mwm:value>\n"
  "    </mwm:toponyms>\n"
  "    <mwm:languageCodes>\n"
  "      <mwm:value>en</mwm:value>\n"
  "      <mwm:value>ja</mwm:value>\n"
  "      <mwm:value>ru</mwm:value>\n"
  "    </mwm:languageCodes>\n"
  "  </ExtendedData>\n"
  "  <Placemark>\n"
  "    <name>Test bookmark</name>\n"
  "    <description>Test bookmark description</description>\n"
  "    <TimeStamp><when>1970-01-01T00:13:20Z</when></TimeStamp>\n"
  "    <styleUrl>#placemark-blue</styleUrl>\n"
  "    <Point><coordinates>45.9242,49.326859</coordinates></Point>\n"
  "    <ExtendedData xmlns:mwm=\"https://maps.me\">\n"
  "      <mwm:name>\n"
  "        <mwm:lang code=\"ru\">Тестовая метка</mwm:lang>\n"
  "      </mwm:name>\n"
  "      <mwm:description>\n"
  "        <mwm:lang code=\"ru\">Тестовое описание метки</mwm:lang>\n"
  "      </mwm:description>\n"
  "      <mwm:types>\n"
  "        <mwm:value>8</mwm:value>\n"
  "        <mwm:value>13</mwm:value>\n"
  "        <mwm:value>34</mwm:value>\n"
  "        <mwm:value>565</mwm:value>\n"
  "      </mwm:types>\n"
  "      <mwm:scale>15</mwm:scale>\n"
  "      <mwm:boundTracks>\n"
  "        <mwm:value>0</mwm:value>\n"
  "      </mwm:boundTracks>\n"
  "    </ExtendedData>\n"
  "  </Placemark>\n"
  "  <Placemark>\n"
  "    <name>Test track</name>\n"
  "    <description>Test track description</description>\n"
  "    <Style><LineStyle>\n"
  "      <color>FF0000FF</color>\n"
  "      <width>6</width>\n"
  "    </LineStyle></Style>\n"
  "    <TimeStamp><when>1970-01-01T00:15:00Z</when></TimeStamp>\n"
  "    <LineString><coordinates>45.9242,49.326859 45.2244,48.941288 45.1964,49.401948</coordinates></LineString>\n"
  "    <ExtendedData xmlns:mwm=\"https://maps.me\">\n"
  "      <mwm:name>\n"
  "        <mwm:lang code=\"ru\">Тестовый трек</mwm:lang>\n"
  "      </mwm:name>\n"
  "      <mwm:description>\n"
  "        <mwm:lang code=\"ru\">Тестовое описание трека</mwm:lang>\n"
  "      </mwm:description>\n"
  "      <mwm:localId>0</mwm:localId>\n"
  "      <mwm:additionalStyle>\n"
  "        <mwm:additionalLineStyle>\n"
  "          <color>FF00FF00</color>\n"
  "          <width>7</width>\n"
  "        </mwm:additionalLineStyle>\n"
  "      </mwm:additionalStyle>\n"
  "    </ExtendedData>\n"
  "  </Placemark>\n"
  "</Document>\n"
  "</kml>";
}  // namespace

// 1. Check text and binary deserialization from the prepared sources in memory.
UNIT_TEST(Kml_Deserialization_Text_Bin_Memory)
{
  UNUSED_VALUE(FormatBytesFromBuffer({}));

  kml::CategoryData dataFromText;
  try
  {
    kml::DeserializerKml des(dataFromText);
    MemReader reader(kTextKml, strlen(kTextKml));
    des.Deserialize(reader);
  }
  catch (kml::DeserializerKml::DeserializeException const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

// TODO: uncomment to output bytes to the log.
//  std::vector<uint8_t> buffer;
//  {
//    kml::binary::SerializerKml ser(dataFromText);
//    MemWriter<decltype(buffer)> sink(buffer);
//    ser.Serialize(sink);
//  }
//  LOG(LINFO, (FormatBytesFromBuffer(buffer)));

  kml::CategoryData dataFromBin;
  try
  {
    MemReader reader(kBinKml.data(), kBinKml.size());
    kml::binary::DeserializerKml des(dataFromBin);
    des.Deserialize(reader);
  }
  catch (kml::binary::DeserializerKml::DeserializeException const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  TEST_EQUAL(dataFromText, dataFromBin, ());
}

// 2. Check text serialization to the memory blob and compare with prepared data.
UNIT_TEST(Kml_Serialization_Text_Memory)
{
  kml::CategoryData data;
  {
    kml::DeserializerKml des(data);
    MemReader reader(kTextKml, strlen(kTextKml));
    des.Deserialize(reader);
  }

  std::string resultBuffer;
  {
    MemWriter<decltype(resultBuffer)> sink(resultBuffer);
    kml::SerializerKml ser(data);
    ser.Serialize(sink);
  }

  kml::CategoryData data2;
  {
    kml::DeserializerKml des(data2);
    MemReader reader(resultBuffer.c_str(), resultBuffer.length());
    des.Deserialize(reader);
  }

  TEST_EQUAL(data, data2, ());
}

// 3. Check binary serialization to the memory blob and compare with prepared data.
UNIT_TEST(Kml_Serialization_Bin_Memory)
{
  kml::CategoryData data;
  {
    kml::binary::DeserializerKml des(data);
    MemReader reader(kBinKml.data(), kBinKml.size());
    des.Deserialize(reader);
  }

  std::vector<uint8_t> buffer;
  {
    kml::binary::SerializerKml ser(data);
    MemWriter<decltype(buffer)> sink(buffer);
    ser.Serialize(sink);
  }

  TEST_EQUAL(kBinKml, buffer, ());

  kml::CategoryData data2;
  {
    kml::binary::DeserializerKml des(data2);
    MemReader reader(buffer.data(), buffer.size());
    des.Deserialize(reader);
  }

  TEST_EQUAL(data, data2, ());
}

// 4. Check deserialization from the text file.
UNIT_TEST(Kml_Deserialization_Text_File)
{
  string const kmlFile = my::JoinFoldersToPath(GetPlatform().TmpDir(), "tmp.kml");
  MY_SCOPE_GUARD(fileGuard, std::bind(&FileWriter::DeleteFileX, kmlFile));
  try
  {
    FileWriter file(kmlFile);
    file.Write(kTextKml, strlen(kTextKml));
  }
  catch (FileWriter::Exception const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  kml::CategoryData dataFromFile;
  try
  {
    kml::DeserializerKml des(dataFromFile);
    FileReader reader(kmlFile);
    des.Deserialize(reader);
  }
  catch (FileReader::Exception const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  kml::CategoryData dataFromText;
  {
    kml::DeserializerKml des(dataFromText);
    MemReader reader(kTextKml, strlen(kTextKml));
    des.Deserialize(reader);
  }

  TEST_EQUAL(dataFromFile, dataFromText, ());
}

// 5. Check deserialization from the binary file.
UNIT_TEST(Kml_Deserialization_Bin_File)
{
  string const kmbFile = my::JoinFoldersToPath(GetPlatform().TmpDir(), "tmp.kmb");
  MY_SCOPE_GUARD(fileGuard, std::bind(&FileWriter::DeleteFileX, kmbFile));
  try
  {
    FileWriter file(kmbFile);
    file.Write(kBinKml.data(), kBinKml.size());
  }
  catch (FileWriter::Exception & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  kml::CategoryData dataFromFile;
  try
  {
    kml::binary::DeserializerKml des(dataFromFile);
    FileReader reader(kmbFile);
    des.Deserialize(reader);
  }
  catch (FileReader::Exception const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  kml::CategoryData dataFromBin;
  {
    kml::binary::DeserializerKml des(dataFromBin);
    MemReader reader(kBinKml.data(), kBinKml.size());
    des.Deserialize(reader);
  }

  TEST_EQUAL(dataFromFile, dataFromBin, ());
}

// 6. Check serialization to the binary file. Here we use generated data.
// The data in RAM must be completely equal to the data in binary file.
UNIT_TEST(Kml_Serialization_Bin_File)
{
  auto data = GenerateCategoryData();

  string const kmbFile = my::JoinFoldersToPath(GetPlatform().TmpDir(), "tmp.kmb");
  MY_SCOPE_GUARD(fileGuard, std::bind(&FileWriter::DeleteFileX, kmbFile));
  try
  {
    kml::binary::SerializerKml ser(data);
    FileWriter writer(kmbFile);
    ser.Serialize(writer);
  }
  catch (FileWriter::Exception const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  kml::CategoryData dataFromFile;
  try
  {
    kml::binary::DeserializerKml des(dataFromFile);
    FileReader reader(kmbFile);
    des.Deserialize(reader);
  }
  catch (FileReader::Exception const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  TEST_EQUAL(data, dataFromFile, ());
}

// 7. Check serialization to the text file. Here we use generated data.
// The text in the file can be not equal to the original generated data, because
// text representation does not support all features, e.g. we do not store ids for
// bookmarks and tracks.
UNIT_TEST(Kml_Serialization_Text_File)
{
  auto data = GenerateCategoryData();

  string const kmlFile = my::JoinFoldersToPath(GetPlatform().TmpDir(), "tmp.kml");
  MY_SCOPE_GUARD(fileGuard, std::bind(&FileWriter::DeleteFileX, kmlFile));
  try
  {
    kml::SerializerKml ser(data);
    FileWriter sink(kmlFile);
    ser.Serialize(sink);
  }
  catch (FileWriter::Exception const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  kml::CategoryData dataFromFile;
  try
  {
    kml::DeserializerKml des(dataFromFile);
    FileReader reader(kmlFile);
    des.Deserialize(reader);
  }
  catch (FileReader::Exception const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  kml::CategoryData dataFromMemory;
  {
    kml::DeserializerKml des(dataFromMemory);
    MemReader reader(kGeneratedKml, strlen(kGeneratedKml));
    des.Deserialize(reader);
  }

  TEST_EQUAL(dataFromFile, dataFromMemory, ());
}
