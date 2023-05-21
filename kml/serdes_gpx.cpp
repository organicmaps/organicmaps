#include "kml/serdes_gpx.hpp"

#include "coding/hex.hpp"
#include "coding/point_coding.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <sstream>


namespace kml
{
namespace gpx
{

using namespace std::string_view_literals;

auto const kDefaultLang = StringUtf8Multilang::kDefaultCode;
auto const kDefaultTrackWidth = 5.0;
std::string_view const kTrk = "trk";
std::string_view const kTrkSeg = "trkseg";
std::string_view const kRte = "rte";
std::string_view const kTrkPt = "trkpt";
std::string_view const kWpt = "wpt";
std::string_view const kRtePt = "rtept";
std::string_view const kName = "name";
std::string_view const kDesc = "desc";


std::string PointToString(m2::PointD const & org)
{
  double const lon = mercator::XToLon(org.x);
  double const lat = mercator::YToLat(org.y);

  std::ostringstream ss;
  ss.precision(8);

  ss << lon << "," << lat;
  return ss.str();
}
}

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
  m_org = {};
  m_predefinedColor = PredefinedColor::None;
  m_color = 0;
  m_customName.clear();
  m_trackLayers.clear();
  m_geometry.Clear();
  m_geometryType = GEOMETRY_TYPE_UNKNOWN;
}

bool GpxParser::MakeValid()
{
  if (GEOMETRY_TYPE_POINT == m_geometryType)
  {
    if (mercator::ValidX(m_org.x) && mercator::ValidY(m_org.y))
    {
      // Set default name.
      if (m_name.empty())
        m_name[gpx::kDefaultLang] = gpx::PointToString(m_org);
      
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

bool GpxParser::Push(std::string_view const & tag)
{
  m_tags.push_back(tag);
  if (GetTagFromEnd(0) == gpx::kWpt)
    m_geometryType = GEOMETRY_TYPE_POINT;
  else if (GetTagFromEnd(0) == gpx::kTrkPt || GetTagFromEnd(0) == gpx::kRtePt)
    m_geometryType = GEOMETRY_TYPE_LINE;
  return true;
}

void GpxParser::AddAttr(std::string const & attr, std::string const & value)
{
  std::string attrInLowerCase = attr;
  strings::AsciiToLower(attrInLowerCase);
  
  if (GetTagFromEnd(0) == gpx::kWpt)
  {
    if (attr == "lat")
      m_lat = stod(value);
    else if (attr == "lon")
      m_lon = stod(value);
  }
  else if ((GetTagFromEnd(0) == gpx::kTrkPt && GetTagFromEnd(1) == gpx::kTrkSeg) ||
           (GetTagFromEnd(0) == gpx::kRtePt && GetTagFromEnd(1) == gpx::kRte))
  {
    if (attr == "lat")
      m_lat = stod(value);
    else if (attr == "lon")
      m_lon = stod(value);
  }
  
}

std::string_view const & GpxParser::GetTagFromEnd(size_t n) const
{
  ASSERT_LESS(n, m_tags.size(), ());
  return m_tags[m_tags.size() - n - 1];
}

void GpxParser::Pop(std::string_view const & tag)
{
  ASSERT_EQUAL(m_tags.back(), tag, ());
  
  if (tag == gpx::kTrkPt || tag == gpx::kRtePt)
  {
    m2::Point p = mercator::FromLatLon(m_lat, m_lon);
    if (m_line.empty() || !AlmostEqualAbs(m_line.back().GetPoint(), p, kMwmPointAccuracy))
      m_line.emplace_back(p);
  }
  else if (tag == gpx::kTrkSeg || tag == gpx::kRte)
  {
    m_geometry.m_lines.push_back(std::move(m_line));
  }
  else if (tag == gpx::kWpt)
  {
    m_org = mercator::FromLatLon(m_lat, m_lon);
  }
  
  if (tag == gpx::kRte || tag == gpx::kTrkSeg || tag == gpx::kWpt)
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
        data.m_point = m_org;
        data.m_customName = std::move(m_customName);

        // Here we set custom name from 'name' field for KML-files exported from 3rd-party services.
        if (data.m_name.size() == 1 && data.m_name.begin()->first == kDefaultLangCode &&
            data.m_customName.empty())
        {
          data.m_customName = data.m_name;
        }

        m_data.m_bookmarksData.push_back(std::move(data));
      }
      else if (GEOMETRY_TYPE_LINE == m_geometryType)
      {
        TrackData data;
        data.m_name = std::move(m_name);
        data.m_description = std::move(m_description);
        data.m_layers = std::move(m_trackLayers);
        data.m_geometry = std::move(m_geometry);
        m_data.m_tracksData.push_back(std::move(data));
      }
    }
    ResetPoint();
  }
  m_tags.pop_back();
}

void GpxParser::CharData(std::string value)
{
  strings::Trim(value);
  
  size_t const count = m_tags.size();
  if (count > 1 && !value.empty())
  {
    std::string_view const & currTag = m_tags[count - 1];
    std::string_view const & prevTag = m_tags[count - 2];

    if (prevTag == gpx::kWpt)
    {
      if (currTag == gpx::kName)
        m_name[gpx::kDefaultLang] = value;
      else if (currTag == gpx::kDesc)
        m_description[gpx::kDefaultLang] = value;
    }
    else if (prevTag == gpx::kTrk || prevTag == gpx::kRte)
    {
      if (currTag == gpx::kName)
        m_categoryData->m_name[gpx::kDefaultLang] = value;
      else if (currTag == gpx::kDesc)
        m_categoryData->m_description[gpx::kDefaultLang] = value;
    }
  }
}

DeserializerGpx::DeserializerGpx(FileData & fileData)
: m_fileData(fileData)
{
  m_fileData = {};
}

} // namespace kml
