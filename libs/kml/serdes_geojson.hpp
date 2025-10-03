#pragma once

#include "kml/types.hpp"

#include <glaze/json.hpp>

#include "base/exception.hpp"

namespace kml
{
namespace geojson
{

// object_t means map<string, json_t>.
typedef glz::json_t::object_t JsonTMap;

// Data structures

struct GeoJsonGeometryPoint
{
  std::string type{"Point"};  // Embedded tag field
  std::vector<double> coordinates;

  friend std::string DebugPrint(GeoJsonGeometryPoint const & c)
  {
    std::ostringstream out;
    out << "GeoJsonGeometryPoint [coordinates = " << c.coordinates.at(1) << ", " << c.coordinates.at(0) << "]";
    return out.str();
  }
};

struct GeoJsonGeometryLine
{
  std::string type{"LineString"};  // Embedded tag field
  std::vector<std::vector<double>> coordinates;

  friend std::string DebugPrint(GeoJsonGeometryLine const & c)
  {
    std::ostringstream out;
    out << "GeoJsonGeometryLine [coordinates = " << c.coordinates.size() << " point(s)]";
    return out.str();
  }
};

struct GeoJsonGeometryUnknown
{
  std::string type;
  glz::json_t coordinates;
};

using GeoJsonGeometry = std::variant<GeoJsonGeometryPoint, GeoJsonGeometryLine, GeoJsonGeometryUnknown>;

std::string DebugPrint(GeoJsonGeometry const & g);

std::string DebugPrint(glz::json_t const & json);

std::string DebugPrint(JsonTMap const & p);

struct GeoJsonFeature
{
  std::string type = "Feature";
  GeoJsonGeometry geometry;
  JsonTMap properties;

  friend std::string DebugPrint(GeoJsonFeature const & c)
  {
    std::ostringstream out;
    out << "GeoJsonFeature [type = " << c.type << ", geometry = " << DebugPrint(c.geometry)
        << ", properties = " << DebugPrint(c.properties) << "]";
    return out.str();
  }
};

struct GeoJsonData
{
  std::string type = "FeatureCollection";
  std::vector<GeoJsonFeature> features;
  std::map<std::string, std::string> properties;
};

// Writer and reader
class GeoJsonWriter
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
  explicit GeojsonParser(FileData & data) : m_fileData(data) {}

  bool Parse(std::string_view json_content);

private:
  FileData & m_fileData;
};

}  // namespace geojson

class DeserializerGeoJson
{
public:
  DECLARE_EXCEPTION(DeserializeException, RootException);

  explicit DeserializerGeoJson(FileData & fileData) : m_fileData(fileData) {}

  void Deserialize(std::string_view content);

private:
  FileData & m_fileData;
};

}  // namespace kml

// Tell Glaze to pick GeoJsonGeometryPoint, GeoJsonGeometryLine or GeoJsonGeometryUnknown depending on "type" property
template <>
struct glz::meta<kml::geojson::GeoJsonGeometry>
{
  static constexpr std::string_view tag = "type";  // Field name that serves as tag
  // TODO: Support Polygon, MultiPoint, MultiLineString, and MultiPolygon
  static constexpr auto ids = std::array{"Point", "LineString"};
};
