#pragma once

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace mvt
{
// Minimal decoder for the Mapbox Vector Tile format (protobuf), version 2.x.
// https://github.com/mapbox/vector-tile-spec

enum class GeomType : uint8_t
{
  Unknown = 0,
  Point = 1,
  LineString = 2,
  Polygon = 3
};

struct Value
{
  enum class Type : uint8_t
  {
    String,
    Double,
    Bool,
    Unsupported
  };

  Type m_type = Type::Unsupported;
  std::string m_string;
  double m_double = 0.0;
  bool m_bool = false;
};

inline std::string DebugPrint(GeomType type)
{
  switch (type)
  {
  case GeomType::Unknown: return "Unknown";
  case GeomType::Point: return "Point";
  case GeomType::LineString: return "LineString";
  case GeomType::Polygon: return "Polygon";
  }
  UNREACHABLE();
}

inline std::string DebugPrint(Value::Type type)
{
  switch (type)
  {
  case Value::Type::String: return "String";
  case Value::Type::Double: return "Double";
  case Value::Type::Bool: return "Bool";
  case Value::Type::Unsupported: return "Unsupported";
  }
  UNREACHABLE();
}

struct Feature
{
  std::vector<std::vector<m2::PointD>> m_lines;       // decoded parts, coordinates in tile units
  std::vector<std::pair<uint32_t, uint32_t>> m_tags;  // (key index, value index) pairs
  GeomType m_geomType = GeomType::Unknown;
};

struct Layer
{
  std::string m_name;
  uint32_t m_extent = 4096;
  std::vector<Feature> m_features;
  std::vector<std::string> m_keys;
  std::vector<Value> m_values;

  // Returns the value of the tag |key| for the feature, or nullptr if absent.
  Value const * GetTag(Feature const & feature, std::string const & key) const;
};

// Decodes a tile into its layers. Returns false on malformed input.
bool Decode(std::string const & data, std::vector<Layer> & layers);
}  // namespace mvt
