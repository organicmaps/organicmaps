#include "testing/testing.hpp"

#include "kml/kml_tests/tests_data.hpp"

#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"

#include "indexer/classificator_loader.hpp"

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
#include <memory>
#include <sstream>
#include <vector>

namespace
{
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
auto const kEnLang = StringUtf8Multilang::kEnglishCode;
auto const kRuLang = static_cast<int8_t>(8);

kml::FileData GenerateKmlFileData()
{
  kml::FileData result;
  result.m_deviceId = "AAAA";
  result.m_serverId = "AAAA-BBBB-CCCC-DDDD";

  result.m_categoryData.m_name[kDefaultLang] = "Test category";
  result.m_categoryData.m_name[kRuLang] = "Тестовая категория";
  result.m_categoryData.m_description[kDefaultLang] = "Test description";
  result.m_categoryData.m_description[kRuLang] = "Тестовое описание";
  result.m_categoryData.m_annotation[kDefaultLang] = "Test annotation";
  result.m_categoryData.m_annotation[kEnLang] = "Test annotation";
  result.m_categoryData.m_imageUrl = "https://localhost/123.png";
  result.m_categoryData.m_visible = true;
  result.m_categoryData.m_authorName = "Organic Maps";
  result.m_categoryData.m_authorId = "12345";
  result.m_categoryData.m_rating = 8.9;
  result.m_categoryData.m_reviewsNumber = 567;
  result.m_categoryData.m_lastModified = kml::TimestampClock::from_time_t(1000);
  result.m_categoryData.m_accessRules = kml::AccessRules::Public;
  result.m_categoryData.m_tags = {"mountains", "ski", "snowboard"};
  result.m_categoryData.m_toponyms = {"12345", "54321"};
  result.m_categoryData.m_languageCodes = {1, 2, 8};
  result.m_categoryData.m_properties = {{"property1", "value1"}, {"property2", "value2"}};

  kml::BookmarkData bookmarkData;
  bookmarkData.m_name[kDefaultLang] = "Test bookmark";
  bookmarkData.m_name[kRuLang] = "Тестовая метка";
  bookmarkData.m_description[kDefaultLang] = "Test bookmark description";
  bookmarkData.m_description[kRuLang] = "Тестовое описание метки";
  bookmarkData.m_featureTypes = {718, 715};
  bookmarkData.m_customName[kDefaultLang] = "Мое любимое место";
  bookmarkData.m_customName[kEnLang] = "My favorite place";
  bookmarkData.m_color = {kml::PredefinedColor::Blue, 0};
  bookmarkData.m_icon = kml::BookmarkIcon::None;
  bookmarkData.m_viewportScale = 15;
  bookmarkData.m_timestamp = kml::TimestampClock::from_time_t(800);
  bookmarkData.m_point = m2::PointD(45.9242, 56.8679);
  bookmarkData.m_boundTracks = {0};
  bookmarkData.m_visible = false;
  bookmarkData.m_nearestToponym = "12345";
  bookmarkData.m_minZoom = 10;
  bookmarkData.m_properties = {{"bm_property1", "value1"},
                               {"bm_property2", "value2"},
                               {"score", "5"}};
  bookmarkData.m_compilations = {1, 2, 3, 4, 5};
  result.m_bookmarksData.emplace_back(std::move(bookmarkData));

  kml::TrackData trackData;
  trackData.m_localId = 0;
  trackData.m_name[kDefaultLang] = "Test track";
  trackData.m_name[kRuLang] = "Тестовый трек";
  trackData.m_description[kDefaultLang] = "Test track description";
  trackData.m_description[kRuLang] = "Тестовое описание трека";
  trackData.m_layers = {{6.0, {kml::PredefinedColor::None, 0xff0000ff}},
                        {7.0, {kml::PredefinedColor::None, 0x00ff00ff}}};
  trackData.m_timestamp = kml::TimestampClock::from_time_t(900);
  trackData.m_pointsWithAltitudes = {{m2::PointD(45.9242, 56.8679), 1},
                                     {m2::PointD(45.2244, 56.2786), 2},
                                     {m2::PointD(45.1964, 56.9832), 3}};
  trackData.m_visible = false;
  trackData.m_nearestToponyms = {"12345", "54321", "98765"};
  trackData.m_properties = {{"tr_property1", "value1"}, {"tr_property2", "value2"}};
  result.m_tracksData.emplace_back(std::move(trackData));

  kml::CategoryData compilationData1;
  compilationData1.m_compilationId = 1;
  compilationData1.m_type = kml::CompilationType::Collection;
  compilationData1.m_name[kDefaultLang] = "Test collection";
  compilationData1.m_name[kRuLang] = "Тестовая коллекция";
  compilationData1.m_description[kDefaultLang] = "Test collection description";
  compilationData1.m_description[kRuLang] = "Тестовое описание коллекции";
  compilationData1.m_annotation[kDefaultLang] = "Test collection annotation";
  compilationData1.m_annotation[kEnLang] = "Test collection annotation";
  compilationData1.m_imageUrl = "https://localhost/1234.png";
  compilationData1.m_visible = true;
  compilationData1.m_authorName = "Organic Maps";
  compilationData1.m_authorId = "54321";
  compilationData1.m_rating = 5.9;
  compilationData1.m_reviewsNumber = 333;
  compilationData1.m_lastModified = kml::TimestampClock::from_time_t(999);
  compilationData1.m_accessRules = kml::AccessRules::Public;
  compilationData1.m_tags = {"mountains", "ski"};
  compilationData1.m_toponyms = {"8", "9"};
  compilationData1.m_languageCodes = {1, 2, 8};
  compilationData1.m_properties = {{"property1", "value1"}, {"property2", "value2"}};
  result.m_compilationsData.push_back(std::move(compilationData1));

  kml::CategoryData compilationData2;
  compilationData2.m_compilationId = 4;
  compilationData2.m_type = kml::CompilationType::Category;
  compilationData2.m_name[kDefaultLang] = "Test category";
  compilationData2.m_name[kRuLang] = "Тестовая категория";
  compilationData2.m_description[kDefaultLang] = "Test category description";
  compilationData2.m_description[kRuLang] = "Тестовое описание категории";
  compilationData2.m_annotation[kDefaultLang] = "Test category annotation";
  compilationData2.m_annotation[kEnLang] = "Test category annotation";
  compilationData2.m_imageUrl = "https://localhost/134.png";
  compilationData2.m_visible = false;
  compilationData2.m_authorName = "Organic Maps";
  compilationData2.m_authorId = "11111";
  compilationData2.m_rating = 3.3;
  compilationData2.m_reviewsNumber = 222;
  compilationData2.m_lastModified = kml::TimestampClock::from_time_t(323);
  compilationData2.m_accessRules = kml::AccessRules::Public;
  compilationData2.m_tags = {"mountains", "bike"};
  compilationData2.m_toponyms = {"10", "11"};
  compilationData2.m_languageCodes = {1, 2, 8};
  compilationData2.m_properties = {{"property1", "value1"}, {"property2", "value2"}};
  result.m_compilationsData.push_back(std::move(compilationData2));

  return result;
}

char const * kGeneratedKml =
R"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://earth.google.com/kml/2.2">
<Document>
  <Style id="placemark-red">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-red.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-blue">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-blue.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-purple">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-purple.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-yellow">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-yellow.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-pink">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-pink.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-brown">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-brown.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-green">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-green.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-orange">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-orange.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-deeppurple">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-deeppurple.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-lightblue">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-lightblue.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-cyan">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-cyan.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-teal">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-teal.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-lime">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-lime.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-deeporange">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-deeporange.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-gray">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-gray.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <Style id="placemark-bluegray">
    <IconStyle>
      <Icon>
        <href>https://omaps.app/placemarks/placemark-bluegray.png</href>
      </Icon>
    </IconStyle>
  </Style>
  <name>Test category</name>
  <description>Test description</description>
  <visibility>1</visibility>
  <ExtendedData xmlns:mwm="https://omaps.app">
    <mwm:serverId>AAAA-BBBB-CCCC-DDDD</mwm:serverId>
    <mwm:name>
      <mwm:lang code="ru">Тестовая категория</mwm:lang>
      <mwm:lang code="default">Test category</mwm:lang>
    </mwm:name>
    <mwm:annotation>
      <mwm:lang code="en">Test annotation</mwm:lang>
      <mwm:lang code="default">Test annotation</mwm:lang>
    </mwm:annotation>
    <mwm:description>
      <mwm:lang code="ru">Тестовое описание</mwm:lang>
      <mwm:lang code="default">Test description</mwm:lang>
    </mwm:description>
    <mwm:imageUrl>https://localhost/123.png</mwm:imageUrl>
    <mwm:author id="12345">Organic Maps</mwm:author>
    <mwm:lastModified>1970-01-01T00:16:40Z</mwm:lastModified>
    <mwm:rating>8.9</mwm:rating>
    <mwm:reviewsNumber>567</mwm:reviewsNumber>
    <mwm:accessRules>Public</mwm:accessRules>
    <mwm:tags>
      <mwm:value>mountains</mwm:value>
      <mwm:value>ski</mwm:value>
      <mwm:value>snowboard</mwm:value>
    </mwm:tags>
    <mwm:toponyms>
      <mwm:value>12345</mwm:value>
      <mwm:value>54321</mwm:value>
    </mwm:toponyms>
    <mwm:languageCodes>
      <mwm:value>en</mwm:value>
      <mwm:value>ja</mwm:value>
      <mwm:value>ru</mwm:value>
    </mwm:languageCodes>
    <mwm:properties>
      <mwm:value key="property1">value1</mwm:value>
      <mwm:value key="property2">value2</mwm:value>
    </mwm:properties>
    <mwm:compilation id="1" type="Collection">
      <mwm:name>
        <mwm:lang code="ru">Тестовая коллекция</mwm:lang>
        <mwm:lang code="default">Test collection</mwm:lang>
      </mwm:name>
      <mwm:annotation>
        <mwm:lang code="en">Test collection annotation</mwm:lang>
        <mwm:lang code="default">Test collection annotation</mwm:lang>
      </mwm:annotation>
      <mwm:description>
        <mwm:lang code="ru">Тестовое описание коллекции</mwm:lang>
        <mwm:lang code="default">Test collection description</mwm:lang>
      </mwm:description>
      <mwm:visibility>1</mwm:visibility>
      <mwm:imageUrl>https://localhost/1234.png</mwm:imageUrl>
      <mwm:author id="54321">Organic Maps</mwm:author>
      <mwm:lastModified>1970-01-01T00:16:39Z</mwm:lastModified>
      <mwm:rating>5.9</mwm:rating>
      <mwm:reviewsNumber>333</mwm:reviewsNumber>
      <mwm:accessRules>Public</mwm:accessRules>
      <mwm:tags>
        <mwm:value>mountains</mwm:value>
        <mwm:value>ski</mwm:value>
      </mwm:tags>
      <mwm:toponyms>
        <mwm:value>8</mwm:value>
        <mwm:value>9</mwm:value>
      </mwm:toponyms>
      <mwm:languageCodes>
        <mwm:value>en</mwm:value>
        <mwm:value>ja</mwm:value>
        <mwm:value>ru</mwm:value>
      </mwm:languageCodes>
      <mwm:properties>
        <mwm:value key="property1">value1</mwm:value>
        <mwm:value key="property2">value2</mwm:value>
      </mwm:properties>
    </mwm:compilation>
    <mwm:compilation id="4" type="Category">
      <mwm:name>
        <mwm:lang code="ru">Тестовая категория</mwm:lang>
        <mwm:lang code="default">Test category</mwm:lang>
      </mwm:name>
      <mwm:annotation>
        <mwm:lang code="en">Test category annotation</mwm:lang>
        <mwm:lang code="default">Test category annotation</mwm:lang>
      </mwm:annotation>
      <mwm:description>
        <mwm:lang code="ru">Тестовое описание категории</mwm:lang>
        <mwm:lang code="default">Test category description</mwm:lang>
      </mwm:description>
      <mwm:visibility>0</mwm:visibility>
      <mwm:imageUrl>https://localhost/134.png</mwm:imageUrl>
      <mwm:author id="11111">Organic Maps</mwm:author>
      <mwm:lastModified>1970-01-01T00:05:23Z</mwm:lastModified>
      <mwm:rating>3.3</mwm:rating>
      <mwm:reviewsNumber>222</mwm:reviewsNumber>
      <mwm:accessRules>Public</mwm:accessRules>
      <mwm:tags>
        <mwm:value>mountains</mwm:value>
        <mwm:value>bike</mwm:value>
      </mwm:tags>
      <mwm:toponyms>
        <mwm:value>10</mwm:value>
        <mwm:value>11</mwm:value>
      </mwm:toponyms>
      <mwm:languageCodes>
        <mwm:value>en</mwm:value>
        <mwm:value>ja</mwm:value>
        <mwm:value>ru</mwm:value>
      </mwm:languageCodes>
      <mwm:properties>
        <mwm:value key="property1">value1</mwm:value>
        <mwm:value key="property2">value2</mwm:value>
      </mwm:properties>
    </mwm:compilation>
  </ExtendedData>
  <Placemark>
    <name>Мое любимое место</name>
    <description>Test bookmark description</description>
    <TimeStamp><when>1970-01-01T00:13:20Z</when></TimeStamp>
    <styleUrl>#placemark-blue</styleUrl>
    <Point><coordinates>45.9242,49.326859</coordinates></Point>
    <ExtendedData xmlns:mwm="https://omaps.app">
      <mwm:name>
        <mwm:lang code="ru">Тестовая метка</mwm:lang>
        <mwm:lang code="default">Test bookmark</mwm:lang>
      </mwm:name>
      <mwm:description>
        <mwm:lang code="ru">Тестовое описание метки</mwm:lang>
        <mwm:lang code="default">Test bookmark description</mwm:lang>
      </mwm:description>
      <mwm:featureTypes>
        <mwm:value>historic-castle</mwm:value>
        <mwm:value>historic-memorial</mwm:value>
      </mwm:featureTypes>
      <mwm:customName>
        <mwm:lang code="en">My favorite place</mwm:lang>
        <mwm:lang code="default">Мое любимое место</mwm:lang>
      </mwm:customName>
      <mwm:scale>15</mwm:scale>
      <mwm:boundTracks>
        <mwm:value>0</mwm:value>
      </mwm:boundTracks>
      <mwm:visibility>0</mwm:visibility>
      <mwm:nearestToponym>12345</mwm:nearestToponym>
      <mwm:minZoom>10</mwm:minZoom>
      <mwm:properties>
        <mwm:value key="bm_property1">value1</mwm:value>
        <mwm:value key="bm_property2">value2</mwm:value>
        <mwm:value key="score">5</mwm:value>
      </mwm:properties>
      <mwm:compilations>1,2,3,4,5</mwm:compilations>
    </ExtendedData>
  </Placemark>
  <Placemark>
    <name>Test track</name>
    <description>Test track description</description>
    <Style><LineStyle>
      <color>FF0000FF</color>
      <width>6</width>
    </LineStyle></Style>
    <TimeStamp><when>1970-01-01T00:15:00Z</when></TimeStamp>
    <LineString><coordinates>45.9242,49.326859,1 45.2244,48.941288,2 45.1964,49.401948,3</coordinates></LineString>
    <ExtendedData xmlns:mwm="https://omaps.app">
      <mwm:name>
        <mwm:lang code="ru">Тестовый трек</mwm:lang>
        <mwm:lang code="default">Test track</mwm:lang>
      </mwm:name>
      <mwm:description>
        <mwm:lang code="ru">Тестовое описание трека</mwm:lang>
        <mwm:lang code="default">Test track description</mwm:lang>
      </mwm:description>
      <mwm:localId>0</mwm:localId>
      <mwm:additionalStyle>
        <mwm:additionalLineStyle>
          <color>FF00FF00</color>
          <width>7</width>
        </mwm:additionalLineStyle>
      </mwm:additionalStyle>
      <mwm:visibility>0</mwm:visibility>
      <mwm:nearestToponyms>
        <mwm:value>12345</mwm:value>
        <mwm:value>54321</mwm:value>
        <mwm:value>98765</mwm:value>
      </mwm:nearestToponyms>
      <mwm:properties>
        <mwm:value key="tr_property1">value1</mwm:value>
        <mwm:value key="tr_property2">value2</mwm:value>
      </mwm:properties>
    </ExtendedData>
  </Placemark>
</Document>
</kml>)";
}  // namespace

// 1. Check text and binary deserialization from the prepared sources in memory.
UNIT_TEST(Kml_Deserialization_Text_Bin_Memory)
{
  UNUSED_VALUE(FormatBytesFromBuffer({}));

  kml::FileData dataFromText;
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

  kml::FileData dataFromBin;
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
  kml::FileData data;
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

  kml::FileData data2;
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
  kml::FileData data;
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

  kml::FileData data2;
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
  std::string const kmlFile = base::JoinPath(GetPlatform().TmpDir(), "tmp.kml");
  SCOPE_GUARD(fileGuard, std::bind(&FileWriter::DeleteFileX, kmlFile));
  try
  {
    FileWriter file(kmlFile);
    file.Write(kTextKml, strlen(kTextKml));
  }
  catch (FileWriter::Exception const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  kml::FileData dataFromFile;
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

  kml::FileData dataFromText;
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
  std::string const kmbFile = base::JoinPath(GetPlatform().TmpDir(), "tmp.kmb");
  SCOPE_GUARD(fileGuard, std::bind(&FileWriter::DeleteFileX, kmbFile));
  try
  {
    FileWriter file(kmbFile);
    file.Write(kBinKml.data(), kBinKml.size());
  }
  catch (FileWriter::Exception & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  kml::FileData dataFromFile;
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

  kml::FileData dataFromBin;
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
  auto data = GenerateKmlFileData();

  std::string const kmbFile = base::JoinPath(GetPlatform().TmpDir(), "tmp.kmb");
  SCOPE_GUARD(fileGuard, std::bind(&FileWriter::DeleteFileX, kmbFile));
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

  kml::FileData dataFromFile;
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
  classificator::Load();

  auto data = GenerateKmlFileData();

  std::string const kmlFile = base::JoinPath(GetPlatform().TmpDir(), "tmp.kml");
  SCOPE_GUARD(fileGuard, std::bind(&FileWriter::DeleteFileX, kmlFile));
  try
  {
    kml::SerializerKml ser(data);
    FileWriter sink(kmlFile);
    ser.Serialize(sink);
  }
  catch (kml::SerializerKml::SerializeException const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }
  catch (FileWriter::Exception const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

// TODO: uncomment to output KML to the log.
//  std::string buffer;
//  {
//    kml::SerializerKml ser(data);
//    MemWriter<decltype(buffer)> sink(buffer);
//    ser.Serialize(sink);
//  }
//  LOG(LINFO, (buffer));

  kml::FileData dataFromFile;
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
  TEST_EQUAL(dataFromFile, data, ());

  kml::FileData dataFromMemory;
  {
    kml::DeserializerKml des(dataFromMemory);
    MemReader reader(kGeneratedKml, strlen(kGeneratedKml));
    des.Deserialize(reader);
  }
  TEST_EQUAL(dataFromMemory, data, ());
}

// 8. Check binary deserialization of v.3 format.
UNIT_TEST(Kml_Deserialization_From_Bin_V3_And_V4)
{
  kml::FileData dataFromBinV3;
  try
  {
    MemReader reader(kBinKmlV3.data(), kBinKmlV3.size());
    kml::binary::DeserializerKml des(dataFromBinV3);
    des.Deserialize(reader);
  }
  catch (kml::binary::DeserializerKml::DeserializeException const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  kml::FileData dataFromBinV4;
  try
  {
    MemReader reader(kBinKmlV4.data(), kBinKmlV4.size());
    kml::binary::DeserializerKml des(dataFromBinV4);
    des.Deserialize(reader);
  }
  catch (kml::binary::DeserializerKml::DeserializeException const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  TEST_EQUAL(dataFromBinV3, dataFromBinV4, ());
}

UNIT_TEST(Kml_Deserialization_From_Bin_V6_And_V7)
{
  kml::FileData dataFromBinV6;
  try
  {
    MemReader reader(kBinKmlV6.data(), kBinKmlV6.size());
    kml::binary::DeserializerKml des(dataFromBinV6);
    des.Deserialize(reader);
  }
  catch (kml::binary::DeserializerKml::DeserializeException const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  kml::FileData dataFromBinV7;
  try
  {
    MemReader reader(kBinKmlV7.data(), kBinKmlV7.size());
    kml::binary::DeserializerKml des(dataFromBinV7);
    des.Deserialize(reader);
  }
  catch (kml::binary::DeserializerKml::DeserializeException const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }

  TEST_EQUAL(dataFromBinV6, dataFromBinV7, ());
}
