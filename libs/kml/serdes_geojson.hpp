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
  using Coordinates = std::variant<std::vector<double>,
                                   std::vector<std::vector<double>>>;

  std::string type;
  Coordinates coordinates;

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

  /*
  void read_coordinates(const glz::json_t& json) {
    if(json.is_array()) {
          if (type == "Point") {
            if (json.get_array().size() != 2) {
                  LOG(LERROR, ("coordinates array has invalid size", json.get_array().size()));
            }
          }
    }
    //TODO
  }*/

  /*template <typename Visitor>
  void Visit(Visitor & visitor) const
  {
    visitor(m_type, "type");
    visitor(m_coordinates, "coordinates");
  }*/

  bool operator==(GeoJsonGeometry const & data) const
  {
      return type == data.type && coordinates == data.coordinates;
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
    return "GeoJsonGeometry [" + c.type + "]";
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
    return type == data.type; //&& m_properties == data.m_properties;
  }

  bool operator!=(GeoJsonFeature const & data) const
  {
    return !operator==(data);
  }

  std::string type = "Feature";
  GeoJsonGeometry geometry;
  glz::json_t properties;

  // Returns 'true' if geometry type is 'Point'.
  bool isPoint();

  // Returns 'true' if geometry type is 'LineString'.
  bool isLine();

  friend std::string DebugPrint(GeoJsonFeature const & c)
  {
      std::ostringstream out;
      out << "[type = " << c.type << ", geometry = " << DebugPrint(c.geometry)
          << ", properties = " /*<< json_dumps(&c.m_properties, JSON_COMPACT)*/ << "]";
      return out.str();
  }
};


struct GeoJsonData
{
  /*DECLARE_VISITOR_AND_DEBUG_PRINT(GeoJsonData,
                                  visitor(m_type, "type"),
                                  visitor(m_features, "features"),
                                  visitor(m_properties, std::map<std::string, std::string>(), "properties"))*/

  bool operator==(GeoJsonData const & data) const
  {
    return type == data.type && features == data.features && properties == data.properties;
  }

  bool operator!=(GeoJsonData const & data) const
  {
    return !operator==(data);
  }

  std::string type = "FeatureCollection";
  std::vector<GeoJsonFeature> features;
  std::map<std::string, std::string> properties;
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


/*
namespace glz
{

using G = kml::geojson::GeoJsonGeometry;

template <>
struct from<JSON, G>
{
    template <auto Opts>
    static void op(G& geometry, is_context auto&& ctx, auto&& it, auto&& end)
    {
        glz::json_t::object_t obj;
        parse<JSON>::op<Opts>(obj, ctx, it, end);

        //uuid = uuid_lib::uuid_from_string_view(str);
        //geometry.type =
    }
};

template <>
struct to<JSON, G>
{
    template <auto Opts>
    static void op(const G& uuid, is_context auto&& ctx, auto&& b, auto&& ix) noexcept
    {
        std::string str = uuid_lib::uuid_to_string(uuid);
        serialize<JSON>::op<Opts>(str, ctx, b, ix);
    }
};
}
*/

/*
template <>
struct glz::meta<kml::geojson::GeoJsonGeometry>
{
    using T = kml::geojson::GeoJsonGeometry;
    static constexpr auto value = object("type", custom<&T::type, &T::type>,
                                         "coordinates", custom<&T::read_coordinates, &T::coordinates>);
};
*/

