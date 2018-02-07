#include "map/bookmark.hpp"
#include "map/api_mark_point.hpp"
#include "map/bookmark_manager.hpp"
#include "map/track.hpp"

#include "base/scope_guard.hpp"

#include "geometry/mercator.hpp"

#include "coding/file_reader.hpp"
#include "coding/hex.hpp"
#include "coding/parse_xml.hpp"  // LoadFromKML
#include "coding/internal/file_data.hpp"

#include "drape/drape_global.hpp"
#include "drape/color.hpp"

#include "drape_frontend/color_constants.hpp"

#include "platform/platform.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>

Bookmark::Bookmark(m2::PointD const & ptOrg)
  : Base(ptOrg, UserMark::BOOKMARK)
  , m_groupId(0)
{}

Bookmark::Bookmark(BookmarkData const & data, m2::PointD const & ptOrg)
  : Base(ptOrg, UserMark::BOOKMARK)
  , m_data(data)
  , m_groupId(0)
{}

void Bookmark::SetData(BookmarkData const & data)
{
  SetDirty();
  m_data = data;
}

BookmarkData const & Bookmark::GetData() const
{
  return m_data;
}

dp::Anchor Bookmark::GetAnchor() const
{
  return dp::Bottom;
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> Bookmark::GetSymbolNames() const
{
  auto const name = GetType();
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  symbol->insert(std::make_pair(1 /* zoomLevel */, name));
  return symbol;
}

bool Bookmark::HasCreationAnimation() const
{
  return true;
}

std::string const & Bookmark::GetName() const
{
  return m_data.GetName();
}

void Bookmark::SetName(std::string const & name)
{
  SetDirty();
  m_data.SetName(name);
}

std::string const & Bookmark::GetType() const
{
  return m_data.GetType();
}

void Bookmark::SetType(std::string const & type)
{
  SetDirty();
  m_data.SetType(type);
}

m2::RectD Bookmark::GetViewport() const
{
  return m2::RectD(GetPivot(), GetPivot());
}

std::string const & Bookmark::GetDescription() const
{
  return m_data.GetDescription();
}

void Bookmark::SetDescription(std::string const & description)
{
  m_data.SetDescription(description);
}

time_t Bookmark::GetTimeStamp() const
{
  return m_data.GetTimeStamp();
}

void Bookmark::SetTimeStamp(time_t timeStamp)
{
  m_data.SetTimeStamp(timeStamp);
}

double Bookmark::GetScale() const
{
  return m_data.GetScale();
}

void Bookmark::SetScale(double scale)
{
  m_data.SetScale(scale);
}

df::MarkGroupID Bookmark::GetGroupId() const
{
  return m_groupId;
}

void Bookmark::Attach(df::MarkGroupID groupID)
{
  ASSERT(!m_groupId, ());
  m_groupId = groupID;
}

void Bookmark::Detach()
{
  m_groupId = 0;
}

BookmarkCategory::BookmarkCategory(std::string const & name,
                                   df::MarkGroupID groupID)
  : Base(UserMark::Type::BOOKMARK)
  , m_groupID(groupID)
  , m_name(name)
{}

BookmarkCategory::~BookmarkCategory()
{
}

void BookmarkCategory::AttachTrack(df::MarkID trackId)
{
  m_tracks.insert(trackId);
}

void BookmarkCategory::DetachTrack(df::MarkID trackId)
{
  m_tracks.erase(trackId);
}

namespace
{
  std::string const kPlacemark = "Placemark";
  std::string const kStyle = "Style";
  std::string const kDocument = "Document";
  std::string const kStyleMap = "StyleMap";
  std::string const kStyleUrl = "styleUrl";
  std::string const kPair = "Pair";

  std::string const kDefaultTrackColor = "DefaultTrackColor";
  float const kDefaultTrackWidth = 5.0f;

  std::string PointToString(m2::PointD const & org)
  {
    double const lon = MercatorBounds::XToLon(org.x);
    double const lat = MercatorBounds::YToLat(org.y);

    ostringstream ss;
    ss.precision(8);

    ss << lon << "," << lat;
    return ss.str();
  }

  enum GeometryType
  {
    GEOMETRY_TYPE_UNKNOWN,
    GEOMETRY_TYPE_POINT,
    GEOMETRY_TYPE_LINE
  };
}

class KMLParser
{
  // Fixes icons which are not supported by MapsWithMe.
  std::string GetSupportedBMType(std::string const & s) const
  {
    // Remove leading '#' symbol.
    ASSERT(!s.empty(), ());
    std::string const result = s.substr(1);
    return style::GetSupportedStyle(result, m_name, style::GetDefaultStyle());
  }

  KMLData & m_data;
  
  std::vector<std::string> m_tags;
  GeometryType m_geometryType;
  m2::PolylineD m_points;
  dp::Color m_trackColor;
  
  std::string m_styleId;
  std::string m_mapStyleId;
  std::string m_styleUrlKey;
  std::map<std::string, dp::Color> m_styleUrl2Color;
  std::map<std::string, std::string> m_mapStyle2Style;
  
  std::string m_name;
  std::string m_type;
  std::string m_description;
  time_t m_timeStamp;
  
  m2::PointD m_org;
  double m_scale;
  
  void Reset()
  {
    m_name.clear();
    m_description.clear();
    m_org = m2::PointD(-1000, -1000);
    m_type.clear();
    m_scale = -1.0;
    m_timeStamp = my::INVALID_TIME_STAMP;
    
    m_trackColor = df::GetColorConstant(kDefaultTrackColor);
    m_styleId.clear();
    m_mapStyleId.clear();
    m_styleUrlKey.clear();
    
    m_points.Clear();
    m_geometryType = GEOMETRY_TYPE_UNKNOWN;
  }
  
  bool ParsePoint(std::string const & s, char const * delim, m2::PointD & pt)
  {
    // order in string is: lon, lat, z
    
    strings::SimpleTokenizer iter(s, delim);
    if (iter)
    {
      double lon;
      if (strings::to_double(*iter, lon) && MercatorBounds::ValidLon(lon) && ++iter)
      {
        double lat;
        if (strings::to_double(*iter, lat) && MercatorBounds::ValidLat(lat))
        {
          pt = MercatorBounds::FromLatLon(lat, lon);
          return true;
        }
        else
          LOG(LWARNING, ("Invalid coordinates", s, "while loading", m_name));
      }
    }
    
    return false;
  }
  
  void SetOrigin(std::string const & s)
  {
    m_geometryType = GEOMETRY_TYPE_POINT;
    
    m2::PointD pt;
    if (ParsePoint(s, ", \n\r\t", pt))
      m_org = pt;
  }
  
  void ParseLineCoordinates(std::string const & s, char const * blockSeparator, char const * coordSeparator)
  {
    m_geometryType = GEOMETRY_TYPE_LINE;
    
    strings::SimpleTokenizer cortegeIter(s, blockSeparator);
    while (cortegeIter)
    {
      m2::PointD pt;
      if (ParsePoint(*cortegeIter, coordSeparator, pt))
      {
        if (m_points.GetSize() == 0 || !(pt - m_points.Back()).IsAlmostZero())
          m_points.Add(pt);
      }
      ++cortegeIter;
    }
  }
  
  bool MakeValid()
  {
    if (GEOMETRY_TYPE_POINT == m_geometryType)
    {
      if (MercatorBounds::ValidX(m_org.x) && MercatorBounds::ValidY(m_org.y))
      {
        // set default name
        if (m_name.empty())
          m_name = PointToString(m_org);
        
        // set default pin
        if (m_type.empty())
          m_type = "placemark-red";
        
        return true;
      }
      return false;
    }
    else if (GEOMETRY_TYPE_LINE == m_geometryType)
    {
      return m_points.GetSize() > 1;
    }
    
    return false;
  }
  
  void ParseColor(std::string const & value)
  {
    std::string fromHex = FromHex(value);
    ASSERT(fromHex.size() == 4, ("Invalid color passed"));
    // Color positions in HEX – aabbggrr
    m_trackColor = dp::Color(fromHex[3], fromHex[2], fromHex[1], fromHex[0]);
  }
  
  bool GetColorForStyle(std::string const & styleUrl, dp::Color & color)
  {
    if (styleUrl.empty())
      return false;
    
    // Remove leading '#' symbol
    auto it = m_styleUrl2Color.find(styleUrl.substr(1));
    if (it != m_styleUrl2Color.end())
    {
      color = it->second;
      return true;
    }
    return false;
  }
  
public:
  KMLParser(KMLData & data)
  : m_data(data)
  {
    Reset();
  }
  
  ~KMLParser()
  {
  }
  
  bool Push(std::string const & name)
  {
    m_tags.push_back(name);
    return true;
  }
  
  void AddAttr(std::string const & attr, std::string const & value)
  {
    std::string attrInLowerCase = attr;
    strings::AsciiToLower(attrInLowerCase);
    
    if (IsValidAttribute(kStyle, value, attrInLowerCase))
      m_styleId = value;
    else if (IsValidAttribute(kStyleMap, value, attrInLowerCase))
      m_mapStyleId = value;
  }
  
  bool IsValidAttribute(std::string const & type, std::string const & value,
                        std::string const & attrInLowerCase) const
  {
    return (GetTagFromEnd(0) == type && !value.empty() && attrInLowerCase == "id");
  }
  
  std::string const & GetTagFromEnd(size_t n) const
  {
    ASSERT_LESS(n, m_tags.size(), ());
    return m_tags[m_tags.size() - n - 1];
  }
  
  void Pop(std::string const & tag)
  {
    ASSERT_EQUAL(m_tags.back(), tag, ());
    
    if (tag == kPlacemark)
    {
      if (MakeValid())
      {
        if (GEOMETRY_TYPE_POINT == m_geometryType)
        {
          m_data.m_bookmarks.emplace_back(std::make_unique<Bookmark>(
            BookmarkData(m_name, m_type, m_description, m_scale, m_timeStamp), m_org));
        }
        else if (GEOMETRY_TYPE_LINE == m_geometryType)
        {
          Track::Params params;
          params.m_colors.push_back({ kDefaultTrackWidth, m_trackColor });
          params.m_name = m_name;

          m_data.m_tracks.emplace_back(std::make_unique<Track>(m_points, params));
        }
      }
      Reset();
    }
    else if (tag == kStyle)
    {
      if (GetTagFromEnd(1) == kDocument)
      {
        if (!m_styleId.empty())
        {
          m_styleUrl2Color[m_styleId] = m_trackColor;
          m_trackColor = df::GetColorConstant(kDefaultTrackColor);
        }
      }
    }
    
    m_tags.pop_back();
  }
  
  void CharData(std::string value)
  {
    strings::Trim(value);
    
    size_t const count = m_tags.size();
    if (count > 1 && !value.empty())
    {
      std::string const & currTag = m_tags[count - 1];
      std::string const & prevTag = m_tags[count - 2];
      std::string const ppTag = count > 3 ? m_tags[count - 3] : std::string();
      
      if (prevTag == kDocument)
      {
        if (currTag == "name")
          m_data.m_name = value;
        else if (currTag == "visibility")
          m_data.m_visible = value == "0" ? false : true;
      }
      else if (prevTag == kPlacemark)
      {
        if (currTag == "name")
          m_name = value;
        else if (currTag == "styleUrl")
        {
          // Bookmark draw style.
          m_type = GetSupportedBMType(value);
          
          // Check if url is in styleMap map.
          if (!GetColorForStyle(value, m_trackColor))
          {
            // Remove leading '#' symbol.
            std::string styleId = m_mapStyle2Style[value.substr(1)];
            if (!styleId.empty())
              GetColorForStyle(styleId, m_trackColor);
          }
        }
        else if (currTag == "description")
          m_description = value;
      }
      else if (prevTag == "LineStyle" && currTag == "color")
      {
        ParseColor(value);
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
        else if (prevTag == "ExtendedData")
        {
          if (currTag == "mwm:scale")
          {
            if (!strings::to_double(value, m_scale))
              m_scale = -1.0;
          }
        }
        else if (prevTag == "TimeStamp")
        {
          if (currTag == "when")
          {
            m_timeStamp = my::StringToTimestamp(value);
            if (m_timeStamp == my::INVALID_TIME_STAMP)
              LOG(LWARNING, ("Invalid timestamp in Placemark:", value));
          }
        }
        else if (currTag == kStyleUrl)
        {
          GetColorForStyle(value, m_trackColor);
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
    }
  }
};

std::string BookmarkCategory::GetDefaultType()
{
  return style::GetDefaultStyle();
}

std::unique_ptr<KMLData> LoadKMLFile(std::string const & file)
{
  auto data = std::make_unique<KMLData>();
  data->m_file = file;
  try
  {
    ReaderSource<ReaderPtr<Reader> > src(std::make_unique<FileReader>(file));
    KMLParser parser(*data);
    if (!ParseXML(src, parser, true))
    {
      LOG(LWARNING, ("XML read error. Probably, incorrect file encoding."));
      data.reset();
    }
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Error while loading bookmarks from", file, e.what()));
    data.reset();
  }
  return data;
}
