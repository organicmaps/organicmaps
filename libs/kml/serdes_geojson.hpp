#pragma once

#include "kml/types.hpp"
#include "kml/color_parser.hpp"

#include "glaze/json.hpp"

#include "base/exception.hpp"
#include "geometry/mercator.hpp"

namespace kml
{
namespace geojson
{

// Data structures

struct GeoJsonGeometry {
  std::string m_type;
  std::vector<m2::PointD> m_coordinates;

  /*template <typename Visitor>
  void Visit(Visitor & visitor)
  {
    visitor(m_type, "type");
    if (m_type == "Point"){
      std::vector<double> coordData;
      visitor(coordData, "coordinates");
      if(coordData.size() != 2) {
        //ERROR!
      }
      m_coordinates.resize(1);
      m_coordinates.at(0) = mercator::FromLatLon(coordData[1], coordData[0]);
    }

    else if (m_type == "LineString") {
      std::vector<std::vector<double>> polygonData;
      visitor(polygonData, "coordinates");
      // Copy coordinates
      m_coordinates.resize(polygonData.size());
      for(size_t i=0; i<polygonData.size(); i++) {
        auto pairCoords = polygonData[i];
        m_coordinates.at(i) = mercator::FromLatLon(pairCoords[1], pairCoords[0]);
      }
    }
    else {
      throw RootException("Unknown GeoJson geometry type", m_type);
    }
  }*/

  /*template <typename Visitor>
  void Visit(Visitor & visitor) const
  {
    visitor(m_type, "type");
    visitor(m_coordinates, "coordinates");
  }*/

  bool operator==(GeoJsonGeometry const & data) const
  {
      return m_type == data.m_type && m_coordinates == data.m_coordinates;
  }

  bool operator!=(GeoJsonGeometry const & data) const
  {
      return !operator==(data);
  }


  friend std::string DebugPrint(GeoJsonGeometry const & c)
  {
    //DebugPrintVisitor visitor("GeoJsonGeometry");
    //c.Visit(visitor);
    //return visitor.ToString();
    return "GeoJsonGeometry [" + c.m_type + "]";
  }
};

struct GeoJsonFeature
{
  /*DECLARE_VISITOR(visitor(m_type, "type"),
                  visitor(m_geometry, "geometry"),
                  visitor(m_properties, *json_object(), "properties"))
  */

  bool operator==(GeoJsonFeature const & data) const
  {
    return m_type == data.m_type; //&& m_properties == data.m_properties;
  }

  bool operator!=(GeoJsonFeature const & data) const
  {
    return !operator==(data);
  }

  std::string m_type = "Feature";
  GeoJsonGeometry m_geometry;
  glz::json_t m_properties;

  // Returns 'true' if geometry type is 'Point'.
  bool isPoint();

  // Returns 'true' if geometry type is 'LineString'.
  bool isLine();

  friend std::string DebugPrint(GeoJsonFeature const & c)
  {
      std::ostringstream out;
      out << "[type = " << c.m_type << ", geometry = " << DebugPrint(c.m_geometry)
          << ", properties = " /*<< json_dumps(&c.m_properties, JSON_COMPACT)*/ << "]";
      return out.str();
  }
};


struct GeoJsonData
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(GeoJsonData,
                                  visitor(m_type, "type"),
                                  visitor(m_features, "features"),
                                  visitor(m_properties, std::map<std::string, std::string>(), "properties"))

  bool operator==(GeoJsonData const & data) const
  {
    return m_type == data.m_type && m_features == data.m_features && m_properties == data.m_properties;
  }

  bool operator!=(GeoJsonData const & data) const
  {
    return !operator==(data);
  }

  std::string m_type = "FeatureCollection";
  std::vector<GeoJsonFeature> m_features;
  std::map<std::string, std::string> m_properties;
};


// Writer and reader
class GeojsonWriter
{
public:
  /*DECLARE_EXCEPTION(WriteGeojsonException, RootException);

  explicit GeojsonWriter(Writer & writer)
    : m_writer(writer)
  {}

  void Write(FileData const & fileData);

private:
  Writer & m_writer;*/
};

class GeojsonParser
{
public:
  explicit GeojsonParser(FileData & data): m_fileData(data) {};

  bool Parse(std::string_view & json_content);

private:
  FileData & m_fileData;
};

}  // namespace geojson

class DeserializerGeoJson
{
public:
    DECLARE_EXCEPTION(DeserializeException, RootException);

    explicit DeserializerGeoJson(FileData & fileData): m_fileData(fileData) {};

    void Deserialize(std::string_view & content);

private:
    FileData & m_fileData;
};


}  // namespace kml
