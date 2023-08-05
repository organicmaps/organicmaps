#pragma once

#include "kml/types.hpp"
#include "kml/serdes_common.hpp"

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
namespace gpx
{
class GpxParser
{
public:
  explicit GpxParser(FileData & data);
  bool Push(std::string name);
  void AddAttr(std::string_view attr, char const * value);
  std::string const & GetTagFromEnd(size_t n) const;
  void Pop(std::string_view tag);
  void CharData(std::string & value);

private:
  enum GeometryType
  {
    GEOMETRY_TYPE_UNKNOWN,
    GEOMETRY_TYPE_POINT,
    GEOMETRY_TYPE_LINE
  };

  void ResetPoint();
  bool MakeValid();
  void ParseColor(std::string const & value);
  void ParseGarminColor(std::string const & value);
  void ParseOsmandColor(std::string const & value);
  bool IsValidCoordinatesPosition();

  FileData & m_data;
  CategoryData m_compilationData;
  CategoryData * m_categoryData;  // never null

  std::vector<std::string> m_tags;
  GeometryType m_geometryType;
  MultiGeometry m_geometry;
  uint32_t m_color;
  uint32_t m_globalColor; // To support OSMAnd extensions with single color per GPX file

  std::string m_name;
  std::string m_description;
  std::string m_comment;
  PredefinedColor m_predefinedColor;
  geometry::PointWithAltitude m_org;

  double m_lat;
  double m_lon;
  geometry::Altitude m_altitude;

  MultiGeometry::LineT m_line;
  std::string m_customName;
  std::vector<TrackLayer> m_trackLayers;
  void ParseName(std::string const & value, std::string const & prevTag);
  void ParseDescription(std::string const & value, std::string const & prevTag);
  void ParseAltitude(std::string const & value);
  std::string BuildDescription();
};
}  // namespace gpx

class DeserializerGpx
{
public:
  DECLARE_EXCEPTION(DeserializeException, RootException);

  explicit DeserializerGpx(FileData & fileData);

  template <typename ReaderType>
  void Deserialize(ReaderType const & reader)
  {
    NonOwningReaderSource src(reader);

    gpx::GpxParser parser(m_fileData);
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
}  // namespace kml
