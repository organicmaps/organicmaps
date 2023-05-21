#pragma once

#include "kml/types.hpp"

#include "coding/parse_xml.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/exception.hpp"
#include "base/stl_helpers.hpp"

#include <chrono>
#include <string>

namespace kml
{

class GpxParser
{
public:
  explicit GpxParser(FileData & data);
  bool Push(std::string_view const & name);
  void AddAttr(std::string const & attr, std::string const & value);
  bool IsValidAttribute(std::string const & type, std::string const & value,
                        std::string const & attrInLowerCase) const;
  std::string_view const & GetTagFromEnd(size_t n) const;
  void Pop(std::string_view const & tag);
  void CharData(std::string value);
  
  static kml::TrackLayer GetDefaultTrackLayer();
  
private:
  enum GeometryType
  {
    GEOMETRY_TYPE_UNKNOWN,
    GEOMETRY_TYPE_POINT,
    GEOMETRY_TYPE_LINE
  };
  
  void ResetPoint();
  void SetOrigin(std::string const & s);
  void ParseLineCoordinates(std::string const & s, char const * blockSeparator,
                            char const * coordSeparator);
  bool MakeValid();
  void ParseColor(std::string const &value);
  bool GetColorForStyle(std::string const & styleUrl, uint32_t & color) const;
  double GetTrackWidthForStyle(std::string const & styleUrl) const;
  
  FileData & m_data;
  CategoryData m_compilationData;
  CategoryData * m_categoryData;  // never null
  
  std::vector<std::string_view> m_tags;
  GeometryType m_geometryType;
  MultiGeometry m_geometry;
  uint32_t m_color;
  
  std::string m_styleId;
  std::string m_mapStyleId;
  std::string m_styleUrlKey;
  std::map<std::string, uint32_t> m_styleUrl2Color;
  std::map<std::string, double> m_styleUrl2Width;
  std::map<std::string, std::string> m_mapStyle2Style;

  int8_t m_attrCode;
  std::string m_attrId;
  std::string m_attrKey;
  
  LocalizableString m_name;
  LocalizableString m_description;
  PredefinedColor m_predefinedColor;
  Timestamp m_timestamp;
  m2::PointD m_org;

  double m_lat;
  double m_lon;

  MultiGeometry::LineT m_line;
  uint8_t m_viewportScale;
  std::vector<uint32_t> m_featureTypes;
  LocalizableString m_customName;
  std::vector<LocalId> m_boundTracks;
  LocalId m_localId;
  BookmarkIcon m_icon;
  std::vector<TrackLayer> m_trackLayers;
  bool m_visible;
  std::string m_nearestToponym;
  std::vector<std::string> m_nearestToponyms;
  int m_minZoom = 1;
  kml::Properties m_properties;
  std::vector<CompilationId> m_compilations;
  double m_trackWidth;
};

class DeserializerGpx
{
public:
  DECLARE_EXCEPTION(DeserializeException, RootException);
  
  explicit DeserializerGpx(FileData & fileData);
  
  template <typename ReaderType>
  void Deserialize(ReaderType const & reader)
  {
    NonOwningReaderSource src(reader);
    
    GpxParser parser(m_fileData);
    if (!ParseXML(src, parser, true))
    {
      // Print corrupted GPX file for debug and restore purposes.
      std::string gpxText;
      reader.ReadAsString(gpxText);
      if (!gpxText.empty() && gpxText[0] == '<')
        LOG(LWARNING, (gpxText));
      MYTHROW(DeserializeException, ("Could not parse GPX."));
    }
  }
  
private:
  FileData & m_fileData;
};
} // namespace kml
