#include "kml/serdes_gpx.hpp"

#include "indexer/classificator.hpp"

#include "coding/hex.hpp"
#include "coding/point_coding.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include <sstream>

using namespace std::string_literals;

namespace kml
{
namespace gpx
{
auto const kDefaultLang = StringUtf8Multilang::kDefaultCode;

auto const kDefaultTrackWidth = 5.0;

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
, m_attrCode(StringUtf8Multilang::kUnsupportedLanguageCode)
{
  ResetPoint();
}

void GpxParser::ResetPoint()
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
  m_visible = true;
  m_nearestToponym.clear();
  m_nearestToponyms.clear();
  m_properties.clear();
  m_localId = 0;
  m_trackLayers.clear();
  m_trackWidth = gpx::kDefaultTrackWidth;
  m_icon = BookmarkIcon::None;
  
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
      if (m_name.empty() && m_featureTypes.empty())
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

bool GpxParser::Push(std::string const & tag)
{
  m_tags.push_back(tag);
  if (GetTagFromEnd(0) == "wpt")
  {
    m_geometryType = GEOMETRY_TYPE_POINT;
  }
  else if (GetTagFromEnd(0) == "trkpt")
  {
    m_geometryType = GEOMETRY_TYPE_LINE;
  }
  return true;
}

void GpxParser::AddAttr(std::string const & attr, std::string const & value)
{
  std::string attrInLowerCase = attr;
  strings::AsciiToLower(attrInLowerCase);
  
  if (GetTagFromEnd(0) == "wpt")
  {
    if (attr == "lat")
    {
      m_lat = stod(value);
    }
    else if (attr == "lon")
    {
      m_lon = stod(value);
    }
  }
  else if (GetTagFromEnd(0) == "trkpt" && GetTagFromEnd(1) == "trkseg")
  {
    if (attr == "lat")
    {
      m_lat = stod(value);
    }
    else if (attr == "lon")
    {
      m_lon = stod(value);
    }
  }
  
}

std::string const & GpxParser::GetTagFromEnd(size_t n) const
{
  ASSERT_LESS(n, m_tags.size(), ());
  return m_tags[m_tags.size() - n - 1];
}

void GpxParser::Pop(std::string const & tag)
{
  ASSERT_EQUAL(m_tags.back(), tag, ());
  
  if (tag == "trkpt")
  {
    m2::Point p = mercator::FromLatLon(m_lat, m_lon);
    
    if (m_line.empty() || !AlmostEqualAbs(m_line.back().GetPoint(), p, kMwmPointAccuracy))
      m_line.emplace_back(p);
    
  }
  else if (tag == "trkseg")
  {
    m_geometry.m_lines.push_back(std::move(m_line));
  }
  else if (tag == "wpt")
  {
    m_org = mercator::FromLatLon(m_lat, m_lon);
  }
  
  if (tag == "trkseg" || tag == "wpt")
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
        data.m_visible = m_visible;
        data.m_nearestToponym = std::move(m_nearestToponym);
        data.m_minZoom = m_minZoom;
        data.m_properties = std::move(m_properties);
        data.m_compilations = std::move(m_compilations);
        
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
        data.m_geometry = std::move(m_geometry);
        data.m_visible = m_visible;
        data.m_nearestToponyms = std::move(m_nearestToponyms);
        data.m_properties = std::move(m_properties);
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
    std::string const & currTag = m_tags[count - 1];
    std::string const & prevTag = m_tags[count - 2];
    std::string const ppTag = count > 2 ? m_tags[count - 3] : std::string();
    std::string const pppTag = count > 3 ? m_tags[count - 4] : std::string();
    std::string const ppppTag = count > 4 ? m_tags[count - 5] : std::string();
    
    if (prevTag == "wpt")
    {
      if (currTag == "name")
      {
        m_name[gpx::kDefaultLang] = value;
      }
      else if (currTag == "desc")
      {
        m_description[gpx::kDefaultLang] = value;
      }
    }
    else if (prevTag == "trk")
    {
      if (currTag == "name")
      {
        m_categoryData->m_name[gpx::kDefaultLang] = value;
      }
      else if (currTag == "desc")
      {
        m_categoryData->m_description[gpx::kDefaultLang] = value;
      }
    }
  }
}

DeserializerGpx::DeserializerGpx(FileData & fileData)
: m_fileData(fileData)
{
  m_fileData = {};
}

} // namespace kml
