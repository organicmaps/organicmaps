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

#include <chrono>
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
  result.m_categoryData.m_authorName = "Maps.Me";
  result.m_categoryData.m_authorId = "12345";
  result.m_categoryData.m_rating = 8.9;
  result.m_categoryData.m_reviewsNumber = 567;
  result.m_categoryData.m_lastModified = std::chrono::system_clock::from_time_t(1000);
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
  bookmarkData.m_timestamp = std::chrono::system_clock::from_time_t(800);
  bookmarkData.m_point = m2::PointD(45.9242, 56.8679);
  bookmarkData.m_boundTracks = {0};
  bookmarkData.m_visible = false;
  bookmarkData.m_nearestToponym = "12345";
  bookmarkData.m_properties = {{"bm_property1", "value1"}, {"bm_property2", "value2"}};
  result.m_bookmarksData.emplace_back(std::move(bookmarkData));

  kml::TrackData trackData;
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
  trackData.m_visible = false;
  trackData.m_nearestToponyms = {"12345", "54321", "98765"};
  trackData.m_properties = {{"tr_property1", "value1"}, {"tr_property2", "value2"}};
  result.m_tracksData.emplace_back(std::move(trackData));

  return result;
}

char const * kGeneratedKml =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
  "<Document>\n"
  "  <Style id=\"placemark-red\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://maps.me/placemarks/placemark-red.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-blue\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://maps.me/placemarks/placemark-blue.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-purple\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://maps.me/placemarks/placemark-purple.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-yellow\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://maps.me/placemarks/placemark-yellow.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-pink\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://maps.me/placemarks/placemark-pink.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-brown\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://maps.me/placemarks/placemark-brown.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-green\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://maps.me/placemarks/placemark-green.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <Style id=\"placemark-orange\">\n"
  "    <IconStyle>\n"
  "      <Icon>\n"
  "        <href>http://maps.me/placemarks/placemark-orange.png</href>\n"
  "      </Icon>\n"
  "    </IconStyle>\n"
  "  </Style>\n"
  "  <name>Test category</name>\n"
  "  <description>Test description</description>\n"
  "  <visibility>1</visibility>\n"
  "  <ExtendedData xmlns:mwm=\"https://maps.me\">\n"
  "    <mwm:serverId>AAAA-BBBB-CCCC-DDDD</mwm:serverId>\n"
  "    <mwm:name>\n"
  "      <mwm:lang code=\"ru\">Тестовая категория</mwm:lang>\n"
  "      <mwm:lang code=\"default\">Test category</mwm:lang>\n"
  "    </mwm:name>\n"
  "    <mwm:annotation>\n"
  "      <mwm:lang code=\"en\">Test annotation</mwm:lang>\n"
  "      <mwm:lang code=\"default\">Test annotation</mwm:lang>\n"
  "    </mwm:annotation>\n"
  "    <mwm:description>\n"
  "      <mwm:lang code=\"ru\">Тестовое описание</mwm:lang>\n"
  "      <mwm:lang code=\"default\">Test description</mwm:lang>\n"
  "    </mwm:description>\n"
  "    <mwm:imageUrl>https://localhost/123.png</mwm:imageUrl>\n"
  "    <mwm:author id=\"12345\">Maps.Me</mwm:author>\n"
  "    <mwm:lastModified>1970-01-01T00:16:40Z</mwm:lastModified>\n"
  "    <mwm:rating>8.9</mwm:rating>\n"
  "    <mwm:reviewsNumber>567</mwm:reviewsNumber>\n"
  "    <mwm:accessRules>Public</mwm:accessRules>\n"
  "    <mwm:tags>\n"
  "      <mwm:value>mountains</mwm:value>\n"
  "      <mwm:value>ski</mwm:value>\n"
  "      <mwm:value>snowboard</mwm:value>\n"
  "    </mwm:tags>\n"
  "    <mwm:toponyms>\n"
  "      <mwm:value>12345</mwm:value>\n"
  "      <mwm:value>54321</mwm:value>\n"
  "    </mwm:toponyms>\n"
  "    <mwm:languageCodes>\n"
  "      <mwm:value>en</mwm:value>\n"
  "      <mwm:value>ja</mwm:value>\n"
  "      <mwm:value>ru</mwm:value>\n"
  "    </mwm:languageCodes>\n"
  "    <mwm:properties>\n"
  "      <mwm:value key=\"property1\">value1</mwm:value>\n"
  "      <mwm:value key=\"property2\">value2</mwm:value>\n"
  "    </mwm:properties>\n"
  "  </ExtendedData>\n"
  "  <Placemark>\n"
  "    <name>Мое любимое место</name>\n"
  "    <description>Test bookmark description</description>\n"
  "    <TimeStamp><when>1970-01-01T00:13:20Z</when></TimeStamp>\n"
  "    <styleUrl>#placemark-blue</styleUrl>\n"
  "    <Point><coordinates>45.9242,49.326859</coordinates></Point>\n"
  "    <ExtendedData xmlns:mwm=\"https://maps.me\">\n"
  "      <mwm:name>\n"
  "        <mwm:lang code=\"ru\">Тестовая метка</mwm:lang>\n"
  "        <mwm:lang code=\"default\">Test bookmark</mwm:lang>\n"
  "      </mwm:name>\n"
  "      <mwm:description>\n"
  "        <mwm:lang code=\"ru\">Тестовое описание метки</mwm:lang>\n"
  "        <mwm:lang code=\"default\">Test bookmark description</mwm:lang>\n"
  "      </mwm:description>\n"
  "      <mwm:featureTypes>\n"
  "        <mwm:value>historic-castle</mwm:value>\n"
  "        <mwm:value>historic-memorial</mwm:value>\n"
  "      </mwm:featureTypes>\n"
  "      <mwm:customName>\n"
  "        <mwm:lang code=\"en\">My favorite place</mwm:lang>\n"
  "        <mwm:lang code=\"default\">Мое любимое место</mwm:lang>\n"
  "      </mwm:customName>\n"
  "      <mwm:scale>15</mwm:scale>\n"
  "      <mwm:boundTracks>\n"
  "        <mwm:value>0</mwm:value>\n"
  "      </mwm:boundTracks>\n"
  "      <mwm:visibility>0</mwm:visibility>\n"
  "      <mwm:nearestToponym>12345</mwm:nearestToponym>\n"
  "      <mwm:properties>\n"
  "        <mwm:value key=\"bm_property1\">value1</mwm:value>\n"
  "        <mwm:value key=\"bm_property2\">value2</mwm:value>\n"
  "      </mwm:properties>\n"
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
  "        <mwm:lang code=\"default\">Test track</mwm:lang>\n"
  "      </mwm:name>\n"
  "      <mwm:description>\n"
  "        <mwm:lang code=\"ru\">Тестовое описание трека</mwm:lang>\n"
  "        <mwm:lang code=\"default\">Test track description</mwm:lang>\n"
  "      </mwm:description>\n"
  "      <mwm:localId>0</mwm:localId>\n"
  "      <mwm:additionalStyle>\n"
  "        <mwm:additionalLineStyle>\n"
  "          <color>FF00FF00</color>\n"
  "          <width>7</width>\n"
  "        </mwm:additionalLineStyle>\n"
  "      </mwm:additionalStyle>\n"
  "      <mwm:visibility>0</mwm:visibility>\n"
  "      <mwm:nearestToponyms>\n"
  "        <mwm:value>12345</mwm:value>\n"
  "        <mwm:value>54321</mwm:value>\n"
  "        <mwm:value>98765</mwm:value>\n"
  "      </mwm:nearestToponyms>\n"
  "      <mwm:properties>\n"
  "        <mwm:value key=\"tr_property1\">value1</mwm:value>\n"
  "        <mwm:value key=\"tr_property2\">value2</mwm:value>\n"
  "      </mwm:properties>\n"
  "    </ExtendedData>\n"
  "  </Placemark>\n"
  "</Document>\n"
  "</kml>";
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
UNIT_TEST(Kml_Deserialization_Bin_V3)
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
