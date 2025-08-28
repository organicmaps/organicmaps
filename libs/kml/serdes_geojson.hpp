#pragma once

#include "kml/types.hpp"

#include <glaze/json.hpp>

#include "base/exception.hpp"

namespace kml
{
namespace geojson
{

// Data structures

struct GeoJsonGeometryPoint
{
  std::vector<double> coordinates;

  bool operator==(GeoJsonGeometryPoint const & data) const { return coordinates == data.coordinates; }

  bool operator!=(GeoJsonGeometryPoint const & data) const { return !operator==(data); }

  friend std::string DebugPrint(GeoJsonGeometryPoint const & c)
  {
    std::ostringstream out;
    out << "GeoJsonGeometryPoint [coordinates = " << c.coordinates.at(1) << ", " << c.coordinates.at(0) << "]";
    return out.str();
  }
};

struct GeoJsonGeometryLine
{
  std::vector<std::vector<double>> coordinates;

  bool operator==(GeoJsonGeometryLine const & data) const { return coordinates == data.coordinates; }

  bool operator!=(GeoJsonGeometryLine const & data) const { return !operator==(data); }

  friend std::string DebugPrint(GeoJsonGeometryLine const & c)
  {
    std::ostringstream out;
    out << "GeoJsonGeometryLine [coordinates = " << c.coordinates.size() << " point(s)]";
    return out.str();
  }
};

using GeoJsonGeometry = std::variant<GeoJsonGeometryPoint, GeoJsonGeometryLine>;

struct GeoJsonFeature
{
  std::string type = "Feature";
  GeoJsonGeometry geometry;
  std::map<std::string, glz::json_t> properties;

  bool operator==(GeoJsonFeature const & data) const
  {
    return type == data.type;  //&& m_properties == data.m_properties;
  }

  bool operator!=(GeoJsonFeature const & data) const { return !operator==(data); }

  // Returns 'true' if geometry type is 'Point'.
  bool isPoint();

  // Returns 'true' if geometry type is 'LineString'.
  bool isLine();

  friend std::string DebugPrint(GeoJsonFeature const & c)
  {
    std::ostringstream out;
    out << "GeoJsonFeature [type = " << c.type  // << ", geometry = " << DebugPrint(c.geometry)
        << ", properties = " /*<< json_dumps(&c.m_properties, JSON_COMPACT)*/ << "]";
    return out.str();
  }
};

struct GeoJsonData
{
  std::string type = "FeatureCollection";
  std::vector<GeoJsonFeature> features;
  std::map<std::string, std::string> properties;

  bool operator==(GeoJsonData const & data) const
  {
    return type == data.type && features == data.features && properties == data.properties;
  }

  bool operator!=(GeoJsonData const & data) const { return !operator==(data); }
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
  explicit GeojsonParser(FileData & data) : m_fileData(data) {}

  bool Parse(std::string_view & json_content);

private:
  FileData & m_fileData;
};

}  // namespace geojson

class DeserializerGeoJson
{
public:
  DECLARE_EXCEPTION(DeserializeException, RootException);

  explicit DeserializerGeoJson(FileData & fileData) : m_fileData(fileData) {}

  void Deserialize(std::string_view & content);

private:
  FileData & m_fileData;
};

}  // namespace kml

/* Glaze setup */

template <>
struct glz::meta<kml::geojson::GeoJsonGeometryPoint>
{
  using T = kml::geojson::GeoJsonGeometryPoint;
  static constexpr auto value = object("coordinates", &T::coordinates);
};

template <>
struct glz::meta<kml::geojson::GeoJsonGeometryLine>
{
  using T = kml::geojson::GeoJsonGeometryLine;
  static constexpr auto value = object("coordinates", &T::coordinates);
};

// Tell Glaze to pick GeoJsonGeometryPoint or GeoJsonGeometryLine depending on "type" property
template <>
struct glz::meta<kml::geojson::GeoJsonGeometry>
{
  static constexpr std::string_view tag = "type";
  static constexpr auto ids = std::array{"Point", "LineString"};
};
