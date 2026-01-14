#pragma once

#include "coding/writer.hpp"
#include "kml/types.hpp"

#include <glaze/json.hpp>

#include "base/exception.hpp"

namespace kml
{
namespace geojson
{

// object_t means map<string, generic_json>.
typedef glz::generic::object_t GenericJsonMap;

std::string DebugPrint(glz::generic const & json);

std::string DebugPrint(GenericJsonMap const & p);

// Data structures

struct GeoJsonGeometryPoint
{
  std::string type{"Point"};  // Embedded tag field
  std::vector<double> coordinates;
};

std::string DebugPrint(GeoJsonGeometryPoint const & c);

struct GeoJsonGeometryLine
{
  std::string type{"LineString"};  // Embedded tag field
  std::vector<std::vector<double>> coordinates;
};

std::string DebugPrint(GeoJsonGeometryLine const & c);

struct GeoJsonGeometryMultiLine
{
  typedef std::vector<std::vector<double>> LineCoords;

  std::string type{"MultiLineString"};  // Embedded tag field
  std::vector<LineCoords> coordinates;
};

std::string DebugPrint(GeoJsonGeometryMultiLine const & c);

struct GeoJsonGeometryUnknown
{
  std::string type;
  glz::generic coordinates;
};

// clang-format off
using GeoJsonGeometry = std::variant<GeoJsonGeometryPoint, GeoJsonGeometryLine, GeoJsonGeometryMultiLine, GeoJsonGeometryUnknown>;
// clang-format on

std::string DebugPrint(GeoJsonGeometry const & g);

struct GeoJsonFeature
{
  std::string type = "Feature";
  GeoJsonGeometry geometry;
  GenericJsonMap properties;
};

std::string DebugPrint(GeoJsonFeature const & c);

struct GeoJsonData
{
  std::string type = "FeatureCollection";
  std::vector<GeoJsonFeature> features;
  std::optional<std::map<std::string, std::string>> properties;
};

// Color convertion functions
std::string ToGeoJsonColor(ColorData color);
std::optional<ColorData> ParseGeoJsonColor(std::string const & color);
PredefinedColor FindPredefinedColor(std::string colorName);

}  // namespace geojson

// Reader and Writer.
class GeoJsonReader
{
public:
  DECLARE_EXCEPTION(DeserializeException, RootException);

  explicit GeoJsonReader(FileData & fileData) : m_fileData(fileData) {}

  void Deserialize(std::string_view content);

private:
  bool Parse(std::string_view jsonContent);
  FileData & m_fileData;
};

class GeoJsonWriter
{
public:
  DECLARE_EXCEPTION(WriteGeoJsonException, RootException);

  explicit GeoJsonWriter(Writer & writer) : m_writer(writer) {}

  void Write(FileData const & fileData, bool minimize_output);

private:
  Writer & m_writer;
};

}  // namespace kml

// Tell Glaze to pick GeoJsonGeometryPoint, GeoJsonGeometryLine or GeoJsonGeometryUnknown depending on "type" property
template <>
struct glz::meta<kml::geojson::GeoJsonGeometry>
{
  static constexpr std::string_view tag = "type";  // Field name that serves as tag
  // TODO: Support Polygon, MultiPoint, and MultiPolygon
  static constexpr auto ids = std::array{"Point", "LineString", "MultiLineString"};
};
