#include "kml/serdes_gpx.hpp"

#include "coding/hex.hpp"
#include "coding/point_coding.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"


namespace kml
{
namespace gpx
{

std::string_view constexpr kTrk = "trk";
std::string_view constexpr kTrkSeg = "trkseg";
std::string_view constexpr kRte = "rte";
std::string_view constexpr kTrkPt = "trkpt";
std::string_view constexpr kWpt = "wpt";
std::string_view constexpr kRtePt = "rtept";
std::string_view constexpr kName = "name";
std::string_view constexpr kColor = "color";
std::string_view constexpr kOsmandColor = "osmand:color";
std::string_view constexpr kGpx = "gpx";
std::string_view constexpr kGarminColor = "gpxx:DisplayColor";
std::string_view constexpr kDesc = "desc";
std::string_view constexpr kMetadata = "metadata";
std::string_view constexpr kEle = "ele";
std::string_view constexpr kCmt = "cmt";
int constexpr kInvalidColor = 0;

GpxParser::GpxParser(FileData & data)
: m_data(data)
, m_categoryData(&m_data.m_categoryData)
{
  ResetPoint();
}

void GpxParser::ResetPoint()
{
  m_name.clear();
  m_description.clear();
  m_comment.clear();
  m_org = {};
  m_predefinedColor = PredefinedColor::None;
  m_color = kInvalidColor;
  m_customName.clear();
  m_trackLayers.clear();
  m_geometry.Clear();
  m_geometryType = GEOMETRY_TYPE_UNKNOWN;
  m_lat = 0.;
  m_lon = 0.;
  m_altitude = geometry::kInvalidAltitude;
}

bool GpxParser::MakeValid()
{
  if (GEOMETRY_TYPE_POINT == m_geometryType)
  {
    m2::PointD const & pt = m_org.GetPoint();
    if (mercator::ValidX(pt.x) && mercator::ValidY(pt.y))
    {
      // Set default name.
      if (m_name.empty())
        m_name = kml::PointToString(m_org);

      // Set default pin.
      if (m_predefinedColor == PredefinedColor::None)
        m_predefinedColor = PredefinedColor::Red;

      return true;
    }
    return false;
  }
  else if (GEOMETRY_TYPE_LINE == m_geometryType)
  {
    return m_geometry.IsValid();
  }

  return false;
}

bool GpxParser::Push(std::string tag)
{
  if (tag == gpx::kWpt)
    m_geometryType = GEOMETRY_TYPE_POINT;
  else if (tag == gpx::kTrkPt || tag == gpx::kRtePt)
    m_geometryType = GEOMETRY_TYPE_LINE;

  m_tags.emplace_back(std::move(tag));

  return true;
}

bool GpxParser::IsValidCoordinatesPosition()
{
  std::string const & lastTag = GetTagFromEnd(0);
  return lastTag == gpx::kWpt
      || (lastTag == gpx::kTrkPt && GetTagFromEnd(1) == gpx::kTrkSeg)
      || (lastTag == gpx::kRtePt && GetTagFromEnd(1) == gpx::kRte);
}

void GpxParser::AddAttr(std::string_view attr, char const * value)
{
  if (IsValidCoordinatesPosition())
  {
    if (attr == "lat" && !strings::to_double(value, m_lat))
      LOG(LERROR, ("Bad gpx latitude:", value));
    else if (attr == "lon" && !strings::to_double(value, m_lon))
      LOG(LERROR, ("Bad gpx longitude:", value));
  }
}

std::string const & GpxParser::GetTagFromEnd(size_t n) const
{
  ASSERT_LESS(n, m_tags.size(), ());
  return m_tags[m_tags.size() - n - 1];
}

void GpxParser::ParseColor(std::string const & value)
{
  auto const colorBytes = FromHex(value);
  if (colorBytes.size() != 3)
  {
    LOG(LWARNING, ("Invalid color value", value));
    return;
  }
  m_color = kml::ToRGBA(colorBytes[0], colorBytes[1], colorBytes[2], (char)255);
}

// https://osmand.net/docs/technical/osmand-file-formats/osmand-gpx/ - "#AARRGGBB" or "#RRGGBB"
void GpxParser::ParseOsmandColor(std::string const & value) {
  if (value.empty())
  {
    LOG(LWARNING, ("Empty color value"));
    return;
  }
  auto const colorBytes = FromHex(value.substr(1, value.size() - 1));
  uint32_t color;
  switch (colorBytes.size())
  {
    case 3:
      color = kml::ToRGBA(colorBytes[0], colorBytes[1], colorBytes[2], (char)255);
      break;
    case 4:
      color = kml::ToRGBA(colorBytes[1], colorBytes[2], colorBytes[3], colorBytes[0]);
      break;
    default:
      LOG(LWARNING, ("Invalid color value", value));
      return;
  }
  if (m_tags.size() > 2 && GetTagFromEnd(2) == gpx::kGpx)
  {
    m_globalColor = color;
    for (auto & track : m_data.m_tracksData)
      for (auto & layer : track.m_layers)
        layer.m_color.m_rgba = m_globalColor;
  }
  else
  {
    m_color = color;
  }
}

// Garmin extensions spec: https://www8.garmin.com/xmlschemas/GpxExtensionsv3.xsd
// Color mapping: https://help.locusmap.eu/topic/extend-garmin-gpx-compatibilty
void GpxParser::ParseGarminColor(std::string const & v)
{
  static std::unordered_map<std::string, std::string> const kGarminToHex = {
      {"Black", "000000"},
      {"DarkRed", "8b0000"},
      {"DarkGreen", "006400"},
      {"DarkYellow", "b5b820"},
      {"DarkBlue", "00008b"},
      {"DarkMagenta", "8b008b"},
      {"DarkCyan", "008b8b"},
      {"LightGray", "cccccc"},
      {"DarkGray", "444444"},
      {"Red", "ff0000"},
      {"Green", "00ff00"},
      {"Yellow", "ffff00"},
      {"Blue", "0000ff"},
      {"Magenta", "ff00ff"},
      {"Cyan", "00ffff"},
      {"White", "ffffff"},
      {"Transparent", "ff0000"}
  };
  auto const it = kGarminToHex.find(v);
  if (it != kGarminToHex.end())
  {
    return ParseColor(it->second);
  }
  else
  {
    LOG(LWARNING, ("Unsupported color value", v));
    return ParseColor("ff0000");  // default to red
  }
}

void GpxParser::Pop(std::string_view tag)
{
  ASSERT_EQUAL(m_tags.back(), tag, ());

  if (tag == gpx::kTrkPt || tag == gpx::kRtePt)
  {
    m2::PointD const p = mercator::FromLatLon(m_lat, m_lon);
    if (m_line.empty() || !AlmostEqualAbs(m_line.back().GetPoint(), p, kMwmPointAccuracy))
      m_line.emplace_back(p, m_altitude);
    m_altitude = geometry::kInvalidAltitude;
  }
  else if (tag == gpx::kTrkSeg || tag == gpx::kRte)
  {
    m_geometry.m_lines.push_back(std::move(m_line));
  }
  else if (tag == gpx::kWpt)
  {
    m_org.SetPoint(mercator::FromLatLon(m_lat, m_lon));
    m_org.SetAltitude(m_altitude);
    m_altitude = geometry::kInvalidAltitude;
  }

  if (tag == gpx::kRte || tag == gpx::kTrk || tag == gpx::kWpt)
  {
    if (MakeValid())
    {
      if (GEOMETRY_TYPE_POINT == m_geometryType)
      {
        BookmarkData data;
        if (!m_name.empty())
          data.m_name = {{ kDefaultLang, m_name }};
        if (!m_description.empty() || !m_comment.empty())
          data.m_description = {{kDefaultLang, BuildDescription()}};
        data.m_color.m_predefinedColor = m_predefinedColor;
        data.m_color.m_rgba = m_color;
        data.m_point = m_org;
        if (!m_customName.empty())
          data.m_customName = {{kDefaultLang, m_customName}};
        // Here we set custom name from 'name' field for KML-files exported from 3rd-party services.
        if (data.m_name.size() == 1 && data.m_name.begin()->first == kDefaultLangCode && data.m_customName.empty())
          data.m_customName = data.m_name;

        m_data.m_bookmarksData.push_back(std::move(data));
      }
      else if (GEOMETRY_TYPE_LINE == m_geometryType)
      {
        TrackLayer layer;
        layer.m_lineWidth = kml::kDefaultTrackWidth;
        if (m_color != kInvalidColor)
          layer.m_color.m_rgba = m_color;
        else if (m_globalColor != kInvalidColor)
          layer.m_color.m_rgba = m_globalColor;
        else
          layer.m_color.m_rgba = kml::kDefaultTrackColor;
        m_trackLayers.push_back(std::move(layer));

        TrackData data;
        if (!m_name.empty())
          data.m_name = {{ kDefaultLang, m_name }};
        if (!m_description.empty() || !m_comment.empty())
          data.m_description = {{kDefaultLang, BuildDescription()}};
        data.m_layers = std::move(m_trackLayers);
        data.m_geometry = std::move(m_geometry);
        m_data.m_tracksData.push_back(std::move(data));
      }
    }
    ResetPoint();
  }
  m_tags.pop_back();
}

void GpxParser::CharData(std::string & value)
{
  strings::Trim(value);

  size_t const count = m_tags.size();
  if (count > 1 && !value.empty())
  {
    std::string const & currTag = m_tags[count - 1];
    std::string const & prevTag = m_tags[count - 2];

    if (currTag == gpx::kName)
      ParseName(value, prevTag);
    else if (currTag == gpx::kDesc)
      ParseDescription(value, prevTag);
    else if (currTag == gpx::kGarminColor)
      ParseGarminColor(value);
    else if (currTag == gpx::kOsmandColor)
      ParseOsmandColor(value);
    else if (currTag == gpx::kColor)
      ParseColor(value);
    else if (currTag == gpx::kEle)
      ParseAltitude(value);
    else if (currTag == gpx::kCmt)
      m_comment = value;
  }
}

void GpxParser::ParseDescription(std::string const & value, std::string const & prevTag)
{
  if (prevTag == kWpt)
  {
    m_description = value;
  }
  else if (prevTag == kTrk || prevTag == kRte)
  {
    m_description = value;
    if (m_categoryData->m_description[kDefaultLang].empty())
      m_categoryData->m_description[kDefaultLang] = value;
  }
  else if (prevTag == kMetadata)
  {
    m_categoryData->m_description[kDefaultLang] = value;
  }
}

void GpxParser::ParseName(std::string const & value, std::string const & prevTag)
{
  if (prevTag == kWpt)
  {
    m_name = value;
  }
  else if (prevTag == kTrk || prevTag == kRte)
  {
    m_name = value;
    if (m_categoryData->m_name[kDefaultLang].empty())
      m_categoryData->m_name[kDefaultLang] = value;
  }
  else if (prevTag == kMetadata)
  {
    m_categoryData->m_name[kDefaultLang] = value;
  }
}

void GpxParser::ParseAltitude(std::string const & value)
{
  double rawAltitude;
  if (strings::to_double(value, rawAltitude))
    m_altitude = static_cast<geometry::Altitude>(round(rawAltitude));
  else
    m_altitude = geometry::kInvalidAltitude;
}

std::string GpxParser::BuildDescription()
{
  if (m_description.empty())
    return m_comment;
  else if (m_comment.empty())
    return m_description;
  else if (m_description == m_comment)
    return m_description;
  return m_description + "\n\n" + m_comment;
}

}  // namespace gpx

DeserializerGpx::DeserializerGpx(FileData & fileData)
: m_fileData(fileData)
{
  m_fileData = {};
}

}  // namespace kml
