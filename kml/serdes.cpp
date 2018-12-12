#include "kml/serdes.hpp"

#include "indexer/classificator.hpp"

#include "geometry/mercator.hpp"

#include "coding/hex.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include <sstream>

using namespace std::string_literals;

namespace kml
{
namespace
{
std::string const kPlacemark = "Placemark";
std::string const kStyle = "Style";
std::string const kDocument = "Document";
std::string const kStyleMap = "StyleMap";
std::string const kStyleUrl = "styleUrl";
std::string const kPair = "Pair";
std::string const kExtendedData = "ExtendedData";

std::string const kKmlHeader =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
    "<Document>\n";

std::string const kKmlFooter =
  "</Document>\n"
    "</kml>\n";

std::string const kExtendedDataHeader =
  "<ExtendedData xmlns:mwm=\"https://maps.me\">\n";

std::string const kExtendedDataFooter =
  "</ExtendedData>\n";

auto const kDefaultLang = StringUtf8Multilang::kDefaultCode;

auto const kDefaultTrackWidth = 5.0;
auto const kDefaultTrackColor = 0x006ec7ff;

std::string Indent(size_t count)
{
  static std::string const kIndent = " ";
  std::ostringstream indent;
  for (size_t i = 0; i < count; ++i)
    indent << kIndent;
  return indent.str();
}

static std::string const kIndent2 = Indent(2);
static std::string const kIndent4 = Indent(4);
static std::string const kIndent6 = Indent(6);
static std::string const kIndent8 = Indent(8);
static std::string const kIndent10 = Indent(10);

std::string PointToString(m2::PointD const & org)
{
  double const lon = MercatorBounds::XToLon(org.x);
  double const lat = MercatorBounds::YToLat(org.y);

  std::ostringstream ss;
  ss.precision(8);

  ss << lon << "," << lat;
  return ss.str();
}

std::string GetLocalizableString(LocalizableString const & s, int8_t lang)
{
  auto const it = s.find(lang);
  if (it == s.cend())
    return {};
  return it->second;
}

PredefinedColor ExtractPlacemarkPredefinedColor(std::string const & s)
{
  if (s == "#placemark-red")
    return PredefinedColor::Red;
  if (s == "#placemark-blue")
    return PredefinedColor::Blue;
  if (s == "#placemark-purple")
    return PredefinedColor::Purple;
  if (s == "#placemark-yellow")
    return PredefinedColor::Yellow;
  if (s == "#placemark-pink")
    return PredefinedColor::Pink;
  if (s == "#placemark-brown")
    return PredefinedColor::Brown;
  if (s == "#placemark-green")
    return PredefinedColor::Green;
  if (s == "#placemark-orange")
    return PredefinedColor::Orange;

  // Default color.
  return PredefinedColor::Red;
}

std::string GetStyleForPredefinedColor(PredefinedColor color)
{
  switch (color)
  {
  case PredefinedColor::Red: return "placemark-red";
  case PredefinedColor::Blue: return "placemark-blue";
  case PredefinedColor::Purple: return "placemark-purple";
  case PredefinedColor::Yellow: return "placemark-yellow";
  case PredefinedColor::Pink: return "placemark-pink";
  case PredefinedColor::Brown: return "placemark-brown";
  case PredefinedColor::Green: return "placemark-green";
  case PredefinedColor::Orange: return "placemark-orange";
  case PredefinedColor::None:
  case PredefinedColor::Count:
    return {};
  }
  UNREACHABLE();
}
  
BookmarkIcon GetIcon(std::string const & iconName)
{
  for (size_t i = 0; i < static_cast<size_t>(BookmarkIcon::Count); ++i)
  {
    auto const icon = static_cast<BookmarkIcon>(i);
    if (iconName == DebugPrint(icon))
      return icon;
  }
  return BookmarkIcon::None;
}

template <typename Channel>
uint32_t ToRGBA(Channel red, Channel green, Channel blue, Channel alpha)
{
  return static_cast<uint8_t>(red) << 24 | static_cast<uint8_t>(green) << 16 |
         static_cast<uint8_t>(blue) << 8 | static_cast<uint8_t>(alpha);
}

void SaveStringWithCDATA(KmlWriter::WriterWrapper & writer, std::string const & s)
{
  if (s.empty())
    return;

  // According to kml/xml spec, we need to escape special symbols with CDATA.
  if (s.find_first_of("<&") != std::string::npos)
    writer << "<![CDATA[" << s << "]]>";
  else
    writer << s;
}

void SaveStyle(KmlWriter::WriterWrapper & writer, std::string const & style)
{
  if (style.empty())
    return;

  writer << kIndent2 << "<Style id=\"" << style << "\">\n"
         << kIndent4 << "<IconStyle>\n"
         << kIndent6 << "<Icon>\n"
         << kIndent8 << "<href>http://maps.me/placemarks/" << style << ".png</href>\n"
         << kIndent6 << "</Icon>\n"
         << kIndent4 << "</IconStyle>\n"
         << kIndent2 << "</Style>\n";
}

void SaveColorToABGR(KmlWriter::WriterWrapper & writer, uint32_t rgba)
{
  writer << NumToHex(static_cast<uint8_t>(rgba & 0xFF))
         << NumToHex(static_cast<uint8_t>((rgba >> 8) & 0xFF))
         << NumToHex(static_cast<uint8_t>((rgba >> 16) & 0xFF))
         << NumToHex(static_cast<uint8_t>((rgba >> 24) & 0xFF));
}

std::string TimestampToString(Timestamp const & timestamp)
{
  auto const ts = std::chrono::system_clock::to_time_t(timestamp);
  std::string const strTimeStamp = base::TimestampToString(ts);
  if (strTimeStamp.size() != 20)
    MYTHROW(KmlWriter::WriteKmlException, ("We always generate fixed length UTC-format timestamp."));
  return strTimeStamp;
}

void SaveLocalizableString(KmlWriter::WriterWrapper & writer, LocalizableString const & str,
                           std::string const & tagName, std::string const & offsetStr)
{
  writer << offsetStr << "<mwm:" << tagName << ">\n";
  for (auto const & s : str)
  {
    writer << offsetStr << kIndent2 << "<mwm:lang code=\""
           << StringUtf8Multilang::GetLangByCode(s.first) << "\">";
    SaveStringWithCDATA(writer, s.second);
    writer << "</mwm:lang>\n";
  }
  writer << offsetStr << "</mwm:" << tagName << ">\n";
}

void SaveStringsArray(KmlWriter::WriterWrapper & writer,
                      std::vector<std::string> const & stringsArray,
                      std::string const & tagName, std::string const & offsetStr)
{
  if (stringsArray.empty())
    return;

  writer << offsetStr << "<mwm:" << tagName << ">\n";
  for (auto const & s : stringsArray)
  {
    writer << offsetStr << kIndent2 << "<mwm:value>";
    SaveStringWithCDATA(writer, s);
    writer << "</mwm:value>\n";
  }
  writer << offsetStr << "</mwm:" << tagName << ">\n";
}

void SavePointsArray(KmlWriter::WriterWrapper & writer,
                     std::vector<m2::PointD> const & pointsArray,
                     std::string const & tagName, std::string const & offsetStr)
{
  if (pointsArray.empty())
    return;

  writer << offsetStr << "<mwm:" << tagName << ">\n";
  for (auto const & p : pointsArray)
  {
    writer << offsetStr << kIndent2 << "<mwm:value>";
    writer << PointToString(p);
    writer << "</mwm:value>\n";
  }
  writer << offsetStr << "</mwm:" << tagName << ">\n";
}

void SaveStringsMap(KmlWriter::WriterWrapper & writer,
                    std::map<std::string, std::string> const & stringsMap,
                    std::string const & tagName, std::string const & offsetStr)
{
  if (stringsMap.empty())
    return;

  writer << offsetStr << "<mwm:" << tagName << ">\n";
  for (auto const & p : stringsMap)
  {
    writer << offsetStr << kIndent2 << "<mwm:value key=\"" << p.first << "\">";
    SaveStringWithCDATA(writer, p.second);
    writer << "</mwm:value>\n";
  }
  writer << offsetStr << "</mwm:" << tagName << ">\n";
}

void SaveCategoryExtendedData(KmlWriter::WriterWrapper & writer, CategoryData const & categoryData,
                              std::string const & extendedServerId)
{
  writer << kIndent2 << kExtendedDataHeader;

  if (!extendedServerId.empty())
    writer << kIndent4 << "<mwm:serverId>" << extendedServerId << "</mwm:serverId>\n";

  SaveLocalizableString(writer, categoryData.m_name, "name", kIndent4);
  SaveLocalizableString(writer, categoryData.m_annotation, "annotation", kIndent4);
  SaveLocalizableString(writer, categoryData.m_description, "description", kIndent4);

  if (!categoryData.m_imageUrl.empty())
    writer << kIndent4 << "<mwm:imageUrl>" << categoryData.m_imageUrl << "</mwm:imageUrl>\n";

  if (!categoryData.m_authorId.empty() || !categoryData.m_authorName.empty())
  {
    writer << kIndent4 << "<mwm:author id=\"" << categoryData.m_authorId << "\">";
    SaveStringWithCDATA(writer, categoryData.m_authorName);
    writer << "</mwm:author>\n";
  }

  if (categoryData.m_lastModified != Timestamp())
  {
    writer << kIndent4 << "<mwm:lastModified>" << TimestampToString(categoryData.m_lastModified)
           << "</mwm:lastModified>\n";
  }

  double constexpr kEps = 1e-5;
  if (fabs(categoryData.m_rating) > kEps)
  {
    writer << kIndent4 << "<mwm:rating>" << strings::to_string(categoryData.m_rating)
           << "</mwm:rating>\n";
  }

  if (categoryData.m_reviewsNumber > 0)
  {
    writer << kIndent4 << "<mwm:reviewsNumber>"
           << strings::to_string(categoryData.m_reviewsNumber) << "</mwm:reviewsNumber>\n";
  }

  writer << kIndent4 << "<mwm:accessRules>" << DebugPrint(categoryData.m_accessRules)
         << "</mwm:accessRules>\n";

  SaveStringsArray(writer, categoryData.m_tags, "tags", kIndent4);

  SavePointsArray(writer, categoryData.m_cities, "cities", kIndent4);

  std::vector<std::string> languageCodes;
  languageCodes.reserve(categoryData.m_languageCodes.size());
  for (auto const & lang : categoryData.m_languageCodes)
  {
    std::string str = StringUtf8Multilang::GetLangByCode(lang);
    if (!str.empty())
      languageCodes.push_back(std::move(str));
  }
  SaveStringsArray(writer, languageCodes, "languageCodes", kIndent4);

  SaveStringsMap(writer, categoryData.m_properties, "properties", kIndent4);

  writer << kIndent2 << kExtendedDataFooter;
}

void SaveCategoryData(KmlWriter::WriterWrapper & writer, CategoryData const & categoryData,
                      std::string const & extendedServerId)
{
  for (uint8_t i = 0; i < base::Key(PredefinedColor::Count); ++i)
    SaveStyle(writer, GetStyleForPredefinedColor(static_cast<PredefinedColor>(i)));

  // Use CDATA if we have special symbols in the name.
  writer << kIndent2 << "<name>";
  SaveStringWithCDATA(writer, GetLocalizableString(categoryData.m_name, kDefaultLang));
  writer << "</name>\n";
  if (!categoryData.m_description.empty())
  {
    writer << kIndent2 << "<description>";
    SaveStringWithCDATA(writer, GetLocalizableString(categoryData.m_description, kDefaultLang));
    writer << "</description>\n";
  }

  writer << kIndent2 << "<visibility>" << (categoryData.m_visible ? "1" : "0") <<"</visibility>\n";

  SaveCategoryExtendedData(writer, categoryData, extendedServerId);
}

void SaveBookmarkExtendedData(KmlWriter::WriterWrapper & writer, BookmarkData const & bookmarkData)
{
  if (bookmarkData.m_name.empty() && bookmarkData.m_description.empty() &&
      bookmarkData.m_customName.empty() && bookmarkData.m_viewportScale == 0 &&
      bookmarkData.m_icon == BookmarkIcon::None && bookmarkData.m_featureTypes.empty() &&
      bookmarkData.m_boundTracks.empty())
  {
    return;
  }

  writer << kIndent4 << kExtendedDataHeader;
  SaveLocalizableString(writer, bookmarkData.m_name, "name", kIndent6);
  SaveLocalizableString(writer, bookmarkData.m_description, "description", kIndent6);

  if (!bookmarkData.m_featureTypes.empty())
  {
    std::vector<std::string> types;
    types.reserve(bookmarkData.m_featureTypes.size());
    auto const & c = classif();
    if (!c.HasTypesMapping())
      MYTHROW(SerializerKml::SerializeException, ("Types mapping is not loaded."));
    for (auto const & t : bookmarkData.m_featureTypes)
      types.push_back(c.GetReadableObjectName(c.GetTypeForIndex(t)));

    SaveStringsArray(writer, types, "featureTypes", kIndent6);
  }

  if (!bookmarkData.m_customName.empty())
    SaveLocalizableString(writer, bookmarkData.m_customName, "customName", kIndent6);

  if (bookmarkData.m_viewportScale != 0)
  {
    auto const scale = strings::to_string(static_cast<double>(bookmarkData.m_viewportScale));
    writer << kIndent6 << "<mwm:scale>" << scale << "</mwm:scale>\n";
  }
  if (bookmarkData.m_icon != BookmarkIcon::None)
    writer << kIndent6 << "<mwm:icon>" << DebugPrint(bookmarkData.m_icon) << "</mwm:icon>\n";

  std::vector<std::string> boundTracks;
  boundTracks.reserve(bookmarkData.m_boundTracks.size());
  for (auto const & t : bookmarkData.m_boundTracks)
    boundTracks.push_back(strings::to_string(static_cast<uint32_t>(t)));
  SaveStringsArray(writer, boundTracks, "boundTracks", kIndent6);

  writer << kIndent4 << kExtendedDataFooter;
}

void SaveBookmarkData(KmlWriter::WriterWrapper & writer, BookmarkData const & bookmarkData)
{
  writer << kIndent2 << "<Placemark>\n";
  writer << kIndent4 << "<name>";
  std::string const defaultLang = StringUtf8Multilang::GetLangByCode(kDefaultLangCode);
  SaveStringWithCDATA(writer, GetPreferredBookmarkName(bookmarkData, defaultLang));
  writer << "</name>\n";

  if (!bookmarkData.m_description.empty())
  {
    writer << kIndent4 << "<description>";
    SaveStringWithCDATA(writer, GetLocalizableString(bookmarkData.m_description, kDefaultLang));
    writer << "</description>\n";
  }

  if (bookmarkData.m_timestamp != Timestamp())
  {
    writer << kIndent4 << "<TimeStamp><when>" << TimestampToString(bookmarkData.m_timestamp)
           << "</when></TimeStamp>\n";
  }

  auto const style = GetStyleForPredefinedColor(bookmarkData.m_color.m_predefinedColor);
  writer << kIndent4 << "<styleUrl>#" << style << "</styleUrl>\n"
         << kIndent4 << "<Point><coordinates>" << PointToString(bookmarkData.m_point)
         << "</coordinates></Point>\n";

  SaveBookmarkExtendedData(writer, bookmarkData);

  writer << kIndent2 << "</Placemark>\n";
}

void SaveTrackLayer(KmlWriter::WriterWrapper & writer, TrackLayer const & layer,
                    std::string const & offsetStr)
{
  writer << offsetStr << "<color>";
  SaveColorToABGR(writer, layer.m_color.m_rgba);
  writer << "</color>\n";
  writer << offsetStr << "<width>" << strings::to_string(layer.m_lineWidth) << "</width>\n";
}

void SaveTrackExtendedData(KmlWriter::WriterWrapper & writer, TrackData const & trackData)
{
  writer << kIndent4 << kExtendedDataHeader;
  SaveLocalizableString(writer, trackData.m_name, "name", kIndent6);
  SaveLocalizableString(writer, trackData.m_description, "description", kIndent6);

  auto const localId = strings::to_string(static_cast<uint32_t>(trackData.m_localId));
  writer << kIndent6 << "<mwm:localId>" << localId << "</mwm:localId>\n";

  writer << kIndent6 << "<mwm:additionalStyle>\n";
  for (size_t i = 1 /* since second layer */; i < trackData.m_layers.size(); ++i)
  {
    writer << kIndent8 << "<mwm:additionalLineStyle>\n";
    SaveTrackLayer(writer, trackData.m_layers[i], kIndent10);
    writer << kIndent8 << "</mwm:additionalLineStyle>\n";
  }
  writer << kIndent6 << "</mwm:additionalStyle>\n";

  writer << kIndent4 << kExtendedDataFooter;
}

void SaveTrackData(KmlWriter::WriterWrapper & writer, TrackData const & trackData)
{
  writer << kIndent2 << "<Placemark>\n";
  writer << kIndent4 << "<name>";
  SaveStringWithCDATA(writer, GetLocalizableString(trackData.m_name, kDefaultLang));
  writer << "</name>\n";

  if (!trackData.m_description.empty())
  {
    writer << kIndent4 << "<description>";
    SaveStringWithCDATA(writer, GetLocalizableString(trackData.m_description, kDefaultLang));
    writer << "</description>\n";
  }

  if (trackData.m_layers.empty())
    MYTHROW(KmlWriter::WriteKmlException, ("Layers list is empty."));

  auto const & layer = trackData.m_layers.front();
  writer << kIndent4 << "<Style><LineStyle>\n";
  SaveTrackLayer(writer, layer, kIndent6);
  writer << kIndent4 << "</LineStyle></Style>\n";

  if (trackData.m_timestamp != Timestamp())
  {
    writer << kIndent4 << "<TimeStamp><when>" << TimestampToString(trackData.m_timestamp)
           << "</when></TimeStamp>\n";
  }

  writer << kIndent4 << "<LineString><coordinates>";
  for (size_t i = 0; i < trackData.m_points.size(); ++i)
  {
    writer << PointToString(trackData.m_points[i]);
    if (i + 1 != trackData.m_points.size())
      writer << " ";
  }
  writer << "</coordinates></LineString>\n";

  SaveTrackExtendedData(writer, trackData);

  writer << kIndent2 << "</Placemark>\n";
}
}  // namespace

KmlWriter::WriterWrapper & KmlWriter::WriterWrapper::operator<<(std::string const & str)
{
  m_writer.Write(str.data(), str.length());
  return *this;
}

void KmlWriter::Write(FileData const & fileData)
{
  m_writer << kKmlHeader;

  // Save category.
  SaveCategoryData(m_writer, fileData.m_categoryData, fileData.m_serverId);

  // Save bookmarks.
  for (auto const & bookmarkData : fileData.m_bookmarksData)
    SaveBookmarkData(m_writer, bookmarkData);

  // Saving tracks.
  for (auto const & trackData : fileData.m_tracksData)
    SaveTrackData(m_writer, trackData);

  m_writer << kKmlFooter;
}

KmlParser::KmlParser(FileData & data)
  : m_data(data)
  , m_attrCode(StringUtf8Multilang::kUnsupportedLanguageCode)
{
  ResetPoint();
}

void KmlParser::ResetPoint()
{
  m_name.clear();
  m_description.clear();
  m_org = {};
  m_predefinedColor = PredefinedColor::None;
  m_viewportScale = 0;
  m_timestamp = {};

  m_color = 0;
  m_styleId.clear();
  m_mapStyleId.clear();
  m_styleUrlKey.clear();

  m_featureTypes.clear();
  m_customName.clear();
  m_boundTracks.clear();
  m_localId = 0;
  m_trackLayers.clear();
  m_trackWidth = kDefaultTrackWidth;
  m_icon = BookmarkIcon::None;

  m_points.clear();
  m_geometryType = GEOMETRY_TYPE_UNKNOWN;
}

bool KmlParser::ParsePoint(std::string const & s, char const * delim, m2::PointD & pt)
{
  // Order in string is: lon, lat, z.
  strings::SimpleTokenizer iter(s, delim);
  if (!iter)
    return false;

  double lon;
  if (strings::to_double(*iter, lon) && MercatorBounds::ValidLon(lon) && ++iter)
  {
    double lat;
    if (strings::to_double(*iter, lat) && MercatorBounds::ValidLat(lat))
    {
      pt = MercatorBounds::FromLatLon(lat, lon);
      return true;
    }
  }
  return false;
}

void KmlParser::SetOrigin(std::string const & s)
{
  m_geometryType = GEOMETRY_TYPE_POINT;

  m2::PointD pt;
  if (ParsePoint(s, ", \n\r\t", pt))
    m_org = pt;
}

void KmlParser::ParseLineCoordinates(std::string const & s, char const * blockSeparator,
                                     char const * coordSeparator)
{
  m_geometryType = GEOMETRY_TYPE_LINE;

  strings::SimpleTokenizer tupleIter(s, blockSeparator);
  while (tupleIter)
  {
    m2::PointD pt;
    if (ParsePoint(*tupleIter, coordSeparator, pt))
    {
      if (m_points.empty() || !pt.EqualDxDy(m_points.back(), 1e-5 /* eps */))
        m_points.push_back(std::move(pt));
    }
    ++tupleIter;
  }
}

bool KmlParser::MakeValid()
{
  if (GEOMETRY_TYPE_POINT == m_geometryType)
  {
    if (MercatorBounds::ValidX(m_org.x) && MercatorBounds::ValidY(m_org.y))
    {
      // Set default name.
      if (m_name.empty() && m_featureTypes.empty())
        m_name[kDefaultLang] = PointToString(m_org);

      // Set default pin.
      if (m_predefinedColor == PredefinedColor::None)
        m_predefinedColor = PredefinedColor::Red;

      return true;
    }
    return false;
  }
  else if (GEOMETRY_TYPE_LINE == m_geometryType)
  {
    return m_points.size() > 1;
  }

  return false;
}

void KmlParser::ParseColor(std::string const & value)
{
  auto const fromHex = FromHex(value);
  if (fromHex.size() != 4)
    return;

  // Color positions in HEX – aabbggrr.
  m_color = ToRGBA(fromHex[3], fromHex[2], fromHex[1], fromHex[0]);
}

bool KmlParser::GetColorForStyle(std::string const & styleUrl, uint32_t & color) const
{
  if (styleUrl.empty())
    return false;

  // Remove leading '#' symbol
  auto const it = m_styleUrl2Color.find(styleUrl.substr(1));
  if (it != m_styleUrl2Color.cend())
  {
    color = it->second;
    return true;
  }
  return false;
}

double KmlParser::GetTrackWidthForStyle(std::string const & styleUrl) const
{
  if (styleUrl.empty())
    return kDefaultTrackWidth;

  // Remove leading '#' symbol
  auto const it = m_styleUrl2Width.find(styleUrl.substr(1));
  if (it != m_styleUrl2Width.cend())
    return it->second;

  return kDefaultTrackWidth;
}

bool KmlParser::Push(std::string const & name)
{
  m_tags.push_back(name);
  return true;
}

void KmlParser::AddAttr(std::string const & attr, std::string const & value)
{
  std::string attrInLowerCase = attr;
  strings::AsciiToLower(attrInLowerCase);

  if (IsValidAttribute(kStyle, value, attrInLowerCase))
    m_styleId = value;
  else if (IsValidAttribute(kStyleMap, value, attrInLowerCase))
    m_mapStyleId = value;

  if (attrInLowerCase == "code")
    m_attrCode = StringUtf8Multilang::GetLangIndex(value);
  else if (attrInLowerCase == "id")
    m_attrId = value;
  else if (attrInLowerCase == "key")
    m_attrKey = value;
}

bool KmlParser::IsValidAttribute(std::string const & type, std::string const & value,
                                 std::string const & attrInLowerCase) const
{
  return (GetTagFromEnd(0) == type && !value.empty() && attrInLowerCase == "id");
}

std::string const & KmlParser::GetTagFromEnd(size_t n) const
{
  ASSERT_LESS(n, m_tags.size(), ());
  return m_tags[m_tags.size() - n - 1];
}

void KmlParser::Pop(std::string const & tag)
{
  ASSERT_EQUAL(m_tags.back(), tag, ());

  if (tag == kPlacemark)
  {
    if (MakeValid())
    {
      if (GEOMETRY_TYPE_POINT == m_geometryType)
      {
        BookmarkData data;
        data.m_name = std::move(m_name);
        data.m_description = std::move(m_description);
        data.m_color.m_predefinedColor = m_predefinedColor;
        data.m_color.m_rgba = m_color;
        data.m_icon = m_icon;
        data.m_viewportScale = m_viewportScale;
        data.m_timestamp = m_timestamp;
        data.m_point = m_org;
        data.m_featureTypes = std::move(m_featureTypes);
        data.m_customName = std::move(m_customName);
        data.m_boundTracks = std::move(m_boundTracks);

        // Here we set custom name from 'name' field for KML-files exported from 3rd-party services.
        if (data.m_name.size() == 1 && data.m_name.begin()->first == kDefaultLangCode &&
            data.m_customName.empty() && data.m_featureTypes.empty())
        {
          data.m_customName = data.m_name;
        }

        m_data.m_bookmarksData.push_back(std::move(data));
      }
      else if (GEOMETRY_TYPE_LINE == m_geometryType)
      {
        TrackData data;
        data.m_localId = m_localId;
        data.m_name = std::move(m_name);
        data.m_description = std::move(m_description);
        data.m_layers = std::move(m_trackLayers);
        data.m_timestamp = m_timestamp;
        data.m_points = m_points;
        m_data.m_tracksData.push_back(std::move(data));
      }
    }
    ResetPoint();
  }
  else if (tag == kStyle)
  {
    if (GetTagFromEnd(1) == kDocument)
    {
      if (!m_styleId.empty())
      {
        m_styleUrl2Color[m_styleId] = m_color;
        m_styleUrl2Width[m_styleId] = m_trackWidth;
        m_color = 0;
        m_trackWidth = kDefaultTrackWidth;
      }
    }
  }
  else if ((tag == "LineStyle" && m_tags.size() > 2 && GetTagFromEnd(2) == kPlacemark) ||
           (tag == "mwm:additionalLineStyle" && m_tags.size() > 3 && GetTagFromEnd(3) == kPlacemark))
  {
    // This code assumes that <Style> is stored inside <Placemark>.
    // It is a violation of KML format, but it must be here to support
    // loading of KML files which were stored by older versions of MAPS.ME.
    TrackLayer layer;
    layer.m_lineWidth = m_trackWidth;
    layer.m_color.m_predefinedColor = PredefinedColor::None;
    layer.m_color.m_rgba = (m_color != 0 ? m_color : kDefaultTrackColor);
    m_trackLayers.push_back(std::move(layer));

    m_trackWidth = kDefaultTrackWidth;
    m_color = 0;
  }

  m_tags.pop_back();
}

void KmlParser::CharData(std::string value)
{
  strings::Trim(value);

  size_t const count = m_tags.size();
  if (count > 1 && !value.empty())
  {
    std::string const & currTag = m_tags[count - 1];
    std::string const & prevTag = m_tags[count - 2];
    std::string const ppTag = count > 2 ? m_tags[count - 3] : std::string();
    std::string const pppTag = count > 3 ? m_tags[count - 4] : std::string();

    if (prevTag == kDocument)
    {
      if (currTag == "name")
        m_data.m_categoryData.m_name[kDefaultLang] = value;
      else if (currTag == "description")
        m_data.m_categoryData.m_description[kDefaultLang] = value;
      else if (currTag == "visibility")
        m_data.m_categoryData.m_visible = value != "0";
    }
    else if (prevTag == kExtendedData && ppTag == kDocument)
    {
      if (currTag == "mwm:author")
      {
        m_data.m_categoryData.m_authorName = value;
        m_data.m_categoryData.m_authorId = m_attrId;
        m_attrId.clear();
      }
      else if (currTag == "mwm:lastModified")
      {
        auto const ts = base::StringToTimestamp(value);
        if (ts != base::INVALID_TIME_STAMP)
          m_data.m_categoryData.m_lastModified = std::chrono::system_clock::from_time_t(ts);
      }
      else if (currTag == "mwm:accessRules")
      {
        // 'Private' is here for back-compatibility.
        if (value == "Private" || value == "Local")
          m_data.m_categoryData.m_accessRules = AccessRules::Local;
        else if (value == "DirectLink")
          m_data.m_categoryData.m_accessRules = AccessRules::DirectLink;
        else if (value == "P2P")
          m_data.m_categoryData.m_accessRules = AccessRules::P2P;
        else if (value == "Paid")
          m_data.m_categoryData.m_accessRules = AccessRules::Paid;
        else if (value == "Public")
          m_data.m_categoryData.m_accessRules = AccessRules::Public;
      }
      else if (currTag == "mwm:imageUrl")
      {
        m_data.m_categoryData.m_imageUrl = value;
      }
      else if (currTag == "mwm:rating")
      {
        if (!strings::to_double(value, m_data.m_categoryData.m_rating))
          m_data.m_categoryData.m_rating = 0.0;
      }
      else if (currTag == "mwm:reviewsNumber")
      {
        if (!strings::to_uint(value, m_data.m_categoryData.m_reviewsNumber))
          m_data.m_categoryData.m_reviewsNumber = 0;
      }
      else if (currTag == "mwm:serverId")
      {
        m_data.m_serverId = value;
      }
    }
    else if (pppTag == kDocument && ppTag == kExtendedData && currTag == "mwm:lang")
    {
      if (prevTag == "mwm:name" && m_attrCode >= 0)
        m_data.m_categoryData.m_name[m_attrCode] = value;
      else if (prevTag == "mwm:description" && m_attrCode >= 0)
        m_data.m_categoryData.m_description[m_attrCode] = value;
      else if (prevTag == "mwm:annotation" && m_attrCode >= 0)
        m_data.m_categoryData.m_annotation[m_attrCode] = value;
      m_attrCode = StringUtf8Multilang::kUnsupportedLanguageCode;
    }
    else if (pppTag == kDocument && ppTag == kExtendedData && currTag == "mwm:value")
    {
      if (prevTag == "mwm:tags")
      {
        m_data.m_categoryData.m_tags.push_back(value);
      }
      else if (prevTag == "mwm:cities")
      {
        m2::PointD pt;
        if (ParsePoint(value, ", \n\r\t", pt))
          m_data.m_categoryData.m_cities.push_back(pt);
      }
      else if (prevTag == "mwm:languageCodes")
      {
        auto const lang = StringUtf8Multilang::GetLangIndex(value);
        if (lang != StringUtf8Multilang::kUnsupportedLanguageCode)
          m_data.m_categoryData.m_languageCodes.push_back(lang);
      }
      else if (prevTag == "mwm:properties" && !m_attrKey.empty())
      {
        m_data.m_categoryData.m_properties[m_attrKey] = value;
        m_attrKey.clear();
      }
    }
    else if (prevTag == kPlacemark)
    {
      if (currTag == "name")
      {
        // We always prefer extended data. There is the following logic of "name" data usage:
        // 1. We have read extended data.
        //   1.1. There is "default" language in extended data (m_name is not empty).
        //        The condition protects us from name rewriting.
        //   1.2. There is no "default" language in extended data. Data from "name" are merged
        //        with extended data. It helps us in the case when we did not save "default"
        //        language in extended data.
        // 2. We have NOT read extended data yet (or at all). In this case m_name must be empty.
        // If extended data will be read, it can rewrite "default" language, since we prefer extended data.
        if (m_name.find(kDefaultLang) == m_name.end())
          m_name[kDefaultLang] = value;
      }
      else if (currTag == kStyleUrl)
      {
        // Bookmark draw style.
        m_predefinedColor = ExtractPlacemarkPredefinedColor(value);
        
        // Here we support old-style hotel placemarks.
        if (value == "#placemark-hotel")
        {
          m_predefinedColor = PredefinedColor::Blue;
          m_icon = BookmarkIcon::Hotel;
        }

        // Track draw style.
        if (!GetColorForStyle(value, m_color))
        {
          // Remove leading '#' symbol.
          std::string styleId = m_mapStyle2Style[value.substr(1)];
          if (!styleId.empty())
            GetColorForStyle(styleId, m_color);
        }
        TrackLayer layer;
        layer.m_lineWidth = GetTrackWidthForStyle(value);
        layer.m_color.m_predefinedColor = PredefinedColor::None;
        layer.m_color.m_rgba = (m_color != 0 ? m_color : kDefaultTrackColor);
        m_trackLayers.push_back(std::move(layer));
      }
      else if (currTag == "description")
      {
        m_description[kDefaultLang] = value;
      }
    }
    else if (prevTag == "LineStyle" || prevTag == "mwm:additionalLineStyle")
    {
      if (currTag == "color")
      {
        ParseColor(value);
      }
      else if (currTag == "width")
      {
        double val;
        if (strings::to_double(value, val))
          m_trackWidth = val;
      }
    }
    else if (ppTag == kStyleMap && prevTag == kPair && currTag == kStyleUrl &&
             m_styleUrlKey == "normal")
    {
      if (!m_mapStyleId.empty())
        m_mapStyle2Style[m_mapStyleId] = value;
    }
    else if (ppTag == kStyleMap && prevTag == kPair && currTag == "key")
    {
      m_styleUrlKey = value;
    }
    else if (ppTag == kPlacemark)
    {
      if (prevTag == "Point")
      {
        if (currTag == "coordinates")
          SetOrigin(value);
      }
      else if (prevTag == "LineString")
      {
        if (currTag == "coordinates")
          ParseLineCoordinates(value, " \n\r\t", ",");
      }
      else if (prevTag == "gx:Track")
      {
        if (currTag == "gx:coord")
          ParseLineCoordinates(value, "\n\r\t", " ");
      }
      else if (prevTag == kExtendedData)
      {
        if (currTag == "mwm:scale")
        {
          double scale;
          if (!strings::to_double(value, scale))
            scale = 0.0;
          m_viewportScale = static_cast<uint8_t>(scale);
        }
        else if (currTag == "mwm:localId")
        {
          uint32_t localId;
          if (!strings::to_uint(value, localId))
            localId = 0;
          m_localId = static_cast<LocalId>(localId);
        }
        else if (currTag == "mwm:icon")
        {
          m_icon = GetIcon(value);
        }
      }
      else if (prevTag == "TimeStamp")
      {
        if (currTag == "when")
        {
          auto const ts = base::StringToTimestamp(value);
          if (ts != base::INVALID_TIME_STAMP)
            m_timestamp = std::chrono::system_clock::from_time_t(ts);
        }
      }
      else if (currTag == kStyleUrl)
      {
        GetColorForStyle(value, m_color);
      }
    }
    else if (ppTag == "MultiGeometry")
    {
      if (prevTag == "Point")
      {
        if (currTag == "coordinates")
          SetOrigin(value);
      }
      else if (prevTag == "LineString")
      {
        if (currTag == "coordinates")
          ParseLineCoordinates(value, " \n\r\t", ",");
      }
      else if (prevTag == "gx:Track")
      {
        if (currTag == "gx:coord")
          ParseLineCoordinates(value, "\n\r\t", " ");
      }
    }
    else if (ppTag == "gx:MultiTrack")
    {
      if (prevTag == "gx:Track")
      {
        if (currTag == "gx:coord")
          ParseLineCoordinates(value, "\n\r\t", " ");
      }
    }
    else if (pppTag == kPlacemark && ppTag == kExtendedData)
    {
      if (currTag == "mwm:lang")
      {
        if (prevTag == "mwm:name" && m_attrCode >= 0)
          m_name[m_attrCode] = value;
        else if (prevTag == "mwm:description" && m_attrCode >= 0)
          m_description[m_attrCode] = value;
        else if (prevTag == "mwm:customName" && m_attrCode >= 0)
          m_customName[m_attrCode] = value;
        m_attrCode = StringUtf8Multilang::kUnsupportedLanguageCode;
      }
      else if (currTag == "mwm:value")
      {
        uint32_t i;
        if (prevTag == "mwm:featureTypes")
        {
          auto const & c = classif();
          if (!c.HasTypesMapping())
            MYTHROW(DeserializerKml::DeserializeException, ("Types mapping is not loaded."));
          auto const type = c.GetTypeByReadableObjectName(value);
          if (c.IsTypeValid(type))
          {
            auto const typeInd = c.GetIndexForType(type);
            m_featureTypes.push_back(typeInd);
          }
        }
        else if (prevTag == "mwm:boundTracks" && strings::to_uint(value, i))
          m_boundTracks.push_back(static_cast<LocalId>(i));
      }
    }
  }
}

// static
kml::TrackLayer KmlParser::GetDefaultTrackLayer()
{
  kml::TrackLayer layer;
  layer.m_lineWidth = kDefaultTrackWidth;
  layer.m_color.m_rgba = kDefaultTrackColor;
  return layer;
}

DeserializerKml::DeserializerKml(FileData & fileData)
  : m_fileData(fileData)
{
  m_fileData = {};
}
}  // namespace kml
