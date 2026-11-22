#include "kml/serdes_gpx.hpp"
#include "kml/color_parser.hpp"
#include "kml/serdes_common.hpp"
#include "kml/type_utils.hpp"

#include "coding/hex.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include <limits>
#include <utility>

namespace kml
{
namespace gpx
{
namespace
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
std::string_view constexpr kTime = "time";

std::string_view constexpr kGpxHeader = R"(<?xml version="1.0"?>
<gpx version="1.1" creator="Organic Maps" xmlns="http://www.topografix.com/GPX/1/1"
    xmlns:gpxx="http://www.garmin.com/xmlschemas/GpxExtensions/v3"
    xmlns:gpx_style="http://www.topografix.com/GPX/gpx_style/0/2"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://www.topografix.com/GPX/1/1 https://www.topografix.com/GPX/1/1/gpx.xsd http://www.topografix.com/GPX/gpx_style/0/2 https://www.topografix.com/GPX/gpx_style/0/2/gpx_style.xsd http://www.garmin.com/xmlschemas/GpxExtensions/v3 https://www.garmin.com/xmlschemas/GpxExtensionsv3.xsd">
)";
std::string_view constexpr kGpxFooter = "</gpx>";

int constexpr kInvalidColor = 0;
}  // namespace

GpxParser::GpxParser(FileData & data) : m_data{data}, m_globalColor{kInvalidColor}
{
  ResetPoint();
}

void GpxParser::ResetPoint()
{
  m_name.clear();
  m_description.clear();
  m_comment.clear();
  m_org = {};
  m_color = kInvalidColor;
  m_geometry.Clear();
  m_geometryType = GEOMETRY_TYPE_UNKNOWN;
  ResetCoordinates();
  m_altitude = geometry::kInvalidAltitude;
  m_timestamp.reset();
  m_rteptName.clear();
  m_stops.clear();
}

// A <wpt>/<trkpt>/<rtept> with a missing or unparseable lat/lon attribute must be skipped instead
// of silently reusing the previous point's coordinates. AddAttr writes m_lat/m_lon only on a
// successful parse, so this out-of-range sentinel survives and IsValidLatLon rejects the point.
void GpxParser::ResetCoordinates()
{
  m_lat = std::numeric_limits<double>::max();
  m_lon = std::numeric_limits<double>::max();
}

bool GpxParser::MakeValid()
{
  if (GEOMETRY_TYPE_POINT == m_geometryType)
  {
    if (IsValidLatLon(m_lat, m_lon))
    {
      // Set default name.
      if (m_name.empty())
        m_name = kml::PointToLineString(m_org);
      return true;
    }
    return false;
  }
  else if (GEOMETRY_TYPE_LINE == m_geometryType)
  {
    return m_geometry.HasLines();
  }

  return false;
}

bool GpxParser::Push(std::string tag)
{
  if (tag == gpx::kWpt)
  {
    m_geometryType = GEOMETRY_TYPE_POINT;
    ResetCoordinates();
  }
  else if (tag == gpx::kTrkPt || tag == gpx::kRtePt)
  {
    m_geometryType = GEOMETRY_TYPE_LINE;
    ResetCoordinates();
  }
  else if (tag == gpx::kTime && !m_tags.empty() && (m_tags.back() == gpx::kTrkPt || m_tags.back() == gpx::kRtePt))
  {
    m_timestamp = base::INVALID_TIME_STAMP;
  }

  m_tags.emplace_back(std::move(tag));

  return true;
}

bool GpxParser::IsValidCoordinatesPosition() const
{
  std::string const & lastTag = GetTagFromEnd(0);
  return lastTag == gpx::kWpt || (lastTag == gpx::kTrkPt && GetTagFromEnd(1) == gpx::kTrkSeg) ||
         (lastTag == gpx::kRtePt && GetTagFromEnd(1) == gpx::kRte);
}

void GpxParser::AddAttr(std::string_view attr, char const * value)
{
  if (IsValidCoordinatesPosition())
  {
    // Yes, https://touren.bodenmais.de/bodenmais/ , we're thinking of you.
    if (*value == '+')
      ++value;
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

std::optional<uint32_t> GpxParser::ParseColorFromHexString(std::string_view colorStr)
{
  auto const res = ParseHexColor(colorStr);
  if (!res)
    LOG(LWARNING, ("Invalid color value", colorStr));
  return res;
}

void GpxParser::ParseColor(std::string_view colorStr)
{
  if (auto const parsed = ParseColorFromHexString(colorStr))
    m_color = *parsed;
}

// https://osmand.net/docs/technical/osmand-file-formats/osmand-gpx/. Supported colors:
// #AARRGGBB/#RRGGBB/AARRGGBB/RRGGBB
void GpxParser::ParseOsmandColor(std::string const & value)
{
  auto const color = ParseColorFromHexString(value);
  if (!color)
    return;

  if (m_tags.size() > 2 && GetTagFromEnd(2) == gpx::kGpx)
  {
    m_globalColor = *color;
    for (auto & track : m_data.m_tracksData)
      for (auto & layer : track.m_layers)
        layer.m_color.m_rgba = m_globalColor;
  }
  else
    m_color = *color;
}

void GpxParser::ParseGarminColor(std::string const & v)
{
  auto const res = kml::ParseGarminColor(v);
  if (!res)
  {
    LOG(LWARNING, ("Unsupported color value", v));
    m_color = ToRGBA(255, 0, 0);  // default to red
  }
  else
    m_color = *res;
}

void GpxParser::Pop(std::string_view tag)
{
  ASSERT_EQUAL(m_tags.back(), tag, ());

  if (tag == gpx::kTrkPt || tag == gpx::kRtePt)
  {
    if (!IsValidLatLon(m_lat, m_lon))
      LOG(LWARNING, ("Skipping gpx point with invalid coordinates:", m_lat, m_lon));
    else
    {
      auto const p = mercator::FromLatLon(m_lat, m_lon);
      m_line.emplace_back(p, m_altitude);
      if (!m_timestamps.empty() || m_timestamp)
      {
        // Keep optional times aligned so compaction can remove gaps on duplicate positions.
        if (m_timestamps.empty())
          m_timestamps.resize(m_line.size() - 1, base::INVALID_TIME_STAMP);
        m_timestamps.push_back(m_timestamp.value_or(base::INVALID_TIME_STAMP));
      }

      if (tag == gpx::kRtePt && GetTagFromEnd(1) == gpx::kRte && m_line.size() > 1)
        m_stops.push_back({p, std::move(m_rteptName)});
    }

    m_altitude = geometry::kInvalidAltitude;
    m_timestamp.reset();
    m_rteptName.clear();
  }
  else if (tag == gpx::kTrkSeg || tag == gpx::kRte)
  {
    // The final valid route point is the finish, even if another stop shares its coordinates.
    if (tag == gpx::kRte && !m_stops.empty())
      m_stops.pop_back();
    if (m_line.size() > 1)
    {
      m_geometry.m_lines.push_back(std::move(m_line));
      m_geometry.m_timestamps.push_back(std::move(m_timestamps));
    }

    // Clear segment (it may be incomplete).
    m_line.clear();
    m_timestamps.clear();
  }
  else if (tag == gpx::kWpt)
  {
    if (IsValidLatLon(m_lat, m_lon))
    {
      m_org.SetPoint(mercator::FromLatLon(m_lat, m_lon));
      m_org.SetAltitude(m_altitude);
    }
    else
      LOG(LWARNING, ("Skipping gpx waypoint with invalid coordinates:", m_lat, m_lon));
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
          data.m_name[kDefaultLang] = std::move(m_name);
        if (!m_description.empty() || !m_comment.empty())
          data.m_description[kDefaultLang] = BuildDescription();
        // A colored waypoint imports as an explicit custom color (forced opaque); a colorless
        // one gets the default preset. See NormalizeBookmarkColorData.
        data.m_color = NormalizeBookmarkColorData({PredefinedColor::None, m_color});
        data.m_point = m_org;
        // A GPX file carries no OM extended data, so its <name> is the only name it has. Store it
        // in m_customName as well: that is the field the place page, search and every exporter
        // treat as the user's own name, and a later rename replaces it instead of leaving two
        // competing names behind. The KML parser copies the name for the same reason, but only for
        // a plain single-language one - which is all this parser ever produces.
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

        TrackData data;
        if (!m_name.empty())
          data.m_name[kDefaultLang] = std::move(m_name);
        if (!m_description.empty() || !m_comment.empty())
          data.m_description[kDefaultLang] = BuildDescription();
        data.m_layers.push_back(layer);
        data.m_geometry = std::move(m_geometry);

        data.m_stops = std::move(m_stops);

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
    else if (currTag == gpx::kEle && (prevTag == gpx::kTrkPt || prevTag == gpx::kRtePt || prevTag == gpx::kWpt))
      ParseAltitude(value);
    else if (currTag == gpx::kCmt)
      m_comment = value;
    else if (currTag == gpx::kTime && (prevTag == gpx::kTrkPt || prevTag == gpx::kRtePt))
      m_timestamp = base::StringToTimestamp(value);
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
    if (m_data.m_categoryData.m_description[kDefaultLang].empty())
      m_data.m_categoryData.m_description[kDefaultLang] = value;
  }
  else if (prevTag == kMetadata)
  {
    m_data.m_categoryData.m_description[kDefaultLang] = value;
  }
}

void GpxParser::ParseName(std::string const & value, std::string const & prevTag)
{
  if (prevTag == kWpt)
  {
    m_name = value;
  }
  else if (prevTag == kRtePt)
  {
    m_rteptName = value;
  }
  else if (prevTag == kTrk || prevTag == kRte)
  {
    m_name = value;
    if (m_data.m_categoryData.m_name[kDefaultLang].empty())
      m_data.m_categoryData.m_name[kDefaultLang] = value;
  }
  else if (prevTag == kMetadata)
  {
    m_data.m_categoryData.m_name[kDefaultLang] = value;
  }
}

void GpxParser::ParseAltitude(std::string const & value)
{
  double rawAltitude;
  if (strings::to_double(value, rawAltitude))
    m_altitude = ToAltitude(rawAltitude);
  else
    m_altitude = geometry::kInvalidAltitude;
}

std::string GpxParser::BuildDescription() const
{
  if (m_description.empty())
    return m_comment;
  else if (m_comment.empty() || m_description == m_comment)
    return m_description;
  return m_description + "\n\n" + m_comment;
}

namespace
{

std::string CoordToString(double coordinate)
{
  // GPX coordinates use xsd:decimal, which requires plain decimal notation.
  return strings::to_string_dac(coordinate, 6);
}

void SaveColorToRGB(Writer & writer, uint32_t rgba)
{
  writer << NumToHex(static_cast<uint8_t>(rgba >> 24 & 0xFF)) << NumToHex(static_cast<uint8_t>((rgba >> 16) & 0xFF))
         << NumToHex(static_cast<uint8_t>((rgba >> 8) & 0xFF));
}

void SaveColorToARGB(Writer & writer, uint32_t rgba)
{
  writer << NumToHex(static_cast<uint8_t>(rgba & 0xFF)) << NumToHex(static_cast<uint8_t>(rgba >> 24 & 0xFF))
         << NumToHex(static_cast<uint8_t>((rgba >> 16) & 0xFF)) << NumToHex(static_cast<uint8_t>((rgba >> 8) & 0xFF));
}

void SaveCategoryData(Writer & writer, CategoryData const & categoryData)
{
  writer << "<metadata>\n";
  if (auto const name = GetStringForExport(categoryData.m_name); !name.empty())
  {
    writer << kIndent2 << "<name>";
    SaveStringWithCDATA(writer, name);
    writer << "</name>\n";
  }
  if (auto const description = GetStringForExport(categoryData.m_description); !description.empty())
  {
    writer << kIndent2 << "<desc>";
    SaveStringWithCDATA(writer, description);
    writer << "</desc>\n";
  }
  writer << "</metadata>\n";
}

uint32_t BookmarkColor(BookmarkData const & bookmarkData)
{
  auto const & [predefinedColor, rgba] = bookmarkData.m_color;
  if (rgba != kInvalidColor)
    return rgba;
  if (predefinedColor != PredefinedColor::None && predefinedColor != PredefinedColor::Red)
    return ColorFromPredefinedColor(predefinedColor).GetRGBA();
  return kInvalidColor;
}

void SaveBookmarkData(Writer & writer, BookmarkData const & bookmarkData)
{
  auto const [lat, lon] = mercator::ToLatLon(bookmarkData.m_point);
  writer << "<wpt lat=\"" << CoordToString(lat) << "\" lon=\"" << CoordToString(lon) << "\">\n";
  if (auto const name = GetPreferredBookmarkName(bookmarkData, "default"); !name.empty())
  {
    writer << kIndent2 << "<name>";
    SaveStringWithCDATA(writer, name);
    writer << "</name>\n";
  }
  if (auto const description = GetStringForExport(bookmarkData.m_description); !description.empty())
  {
    writer << kIndent2 << "<desc>";
    SaveStringWithCDATA(writer, description);
    writer << "</desc>\n";
  }
  if (auto const color = BookmarkColor(bookmarkData); color != kInvalidColor)
  {
    writer << kIndent2 << "<extensions>\n";
    writer << kIndent4 << "<xsi:gpx><color>#";
    SaveColorToARGB(writer, color);
    writer << "</color></xsi:gpx>\n";
    writer << kIndent2 << "</extensions>\n";
  }
  writer << "</wpt>\n";
}

// Track stops are exported as plain waypoints readable by other apps (GPX has no notion of
// per-track stops), so reimporting yields bookmarks, not stops. KML keeps full fidelity.
void SaveTrackStopData(Writer & writer, TrackStopData const & stop)
{
  BookmarkData bookmarkData;
  bookmarkData.m_point = stop.m_point;
  SetDefaultStr(bookmarkData.m_name, stop.m_name);
  SaveBookmarkData(writer, bookmarkData);
}

uint32_t TrackColor(TrackData const & trackData)
{
  if (trackData.m_layers.empty())
    return kDefaultTrackColor;
  return trackData.m_layers.front().m_color.m_rgba;
}

void SaveTrackData(Writer & writer, TrackData const & trackData)
{
  writer << "<trk>\n";
  if (auto const name = GetStringForExport(trackData.m_name); !name.empty())
  {
    writer << kIndent2 << "<name>";
    SaveStringWithCDATA(writer, name);
    writer << "</name>\n";
  }
  if (auto const description = GetStringForExport(trackData.m_description); !description.empty())
  {
    writer << kIndent2 << "<desc>";
    SaveStringWithCDATA(writer, description);
    writer << "</desc>\n";
  }
  if (auto const color = TrackColor(trackData); color != kDefaultTrackColor)
  {
    writer << kIndent2 << "<extensions>\n";
    writer << kIndent4 << "<gpxx:TrackExtension><gpxx:DisplayColor>";
    writer << kml::MapGarminColor(color);
    writer << "</gpxx:DisplayColor></gpxx:TrackExtension>\n";
    writer << kIndent4 << "<gpx_style:line><gpx_style:color>";
    SaveColorToRGB(writer, color);
    writer << "</gpx_style:color></gpx_style:line>\n";
    writer << kIndent4 << "<xsi:gpx><color>#";
    SaveColorToARGB(writer, color);
    writer << "</color></xsi:gpx>\n";
    writer << kIndent2 << "</extensions>\n";
  }
  auto const & geom = trackData.m_geometry;
  ASSERT(geom.IsValid(), ());
  for (size_t lineIndex = 0; lineIndex < geom.m_lines.size(); ++lineIndex)
  {
    auto const & line = geom.m_lines[lineIndex];
    auto const & timestampsForLine = geom.m_timestamps[lineIndex];
    writer << kIndent2 << "<trkseg>\n";

    for (size_t pointIndex = 0; pointIndex < line.size(); ++pointIndex)
    {
      auto const & point = line[pointIndex];
      auto const [lat, lon] = mercator::ToLatLon(point);

      writer << kIndent4 << "<trkpt lat=\"" << CoordToString(lat) << "\" lon=\"" << CoordToString(lon) << "\">\n";

      if (auto const altitude = point.GetAltitude(); altitude != geometry::kInvalidAltitude)
        writer << kIndent6 << "<ele>" << std::to_string(altitude) << "</ele>\n";
      if (!timestampsForLine.empty())
        writer << kIndent6 << "<time>" << base::SecondsSinceEpochToString(timestampsForLine[pointIndex]) << "</time>\n";

      writer << kIndent4 << "</trkpt>\n";
    }
    writer << kIndent2 << "</trkseg>\n";
  }
  writer << "</trk>\n";
}
}  // namespace

void GpxWriter::Write(FileData const & fileData)
{
  if (!fileData.m_areasData.empty())
    MYTHROW(WriteGpxException, ("GPX cannot store polygon areas. Use KML or KMZ."));
  m_writer << kGpxHeader;
  SaveCategoryData(m_writer, fileData.m_categoryData);
  for (auto const & bookmarkData : fileData.m_bookmarksData)
    SaveBookmarkData(m_writer, bookmarkData);
  // The GPX schema requires wpt* elements before trk*, so track stops are written here.
  for (auto const & trackData : fileData.m_tracksData)
    for (auto const & stop : trackData.m_stops)
      SaveTrackStopData(m_writer, stop);
  for (auto const & trackData : fileData.m_tracksData)
    SaveTrackData(m_writer, trackData);
  m_writer << kGpxFooter;
}

}  // namespace gpx

DeserializerGpx::DeserializerGpx(FileData & fileData) : m_fileData(fileData)
{
  m_fileData = {};
}

}  // namespace kml
