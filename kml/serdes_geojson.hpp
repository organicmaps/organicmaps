#pragma once

#include "kml/types.hpp"

#include "coding/writer.hpp"
#include "coding/reader.hpp"
#include "coding/serdes_json.hpp"

#include "base/exception.hpp"

namespace kml
{
namespace geojson
{

// Data structures

struct GeoJsonGeometry {
  std::vector<geometry::PointWithAltitude> m_coordinates;

  template <typename Visitor>
  void Visit(Visitor & visitor)
  {
    try {
      std::vector<double> coordData;
      visitor(coordData, "coordinates");
      if(coordData.size() != 2) {
        //ERROR!
      }
    }
    catch(RootException exc) {
      std::vector<std::pair<double, double>> polygonData;
      visitor(polygonData, "coordinates");
    }
  }

  template <typename Visitor>
  void Visit(Visitor & visitor) const
  {
    std::vector<double> coordData;
    visitor(coordData, "coordinates");
    // TODO
  }

  friend std::string DebugPrint(GeoJsonGeometry const & c)
  {
    DebugPrintVisitor visitor("GeoJsonGeometry");
    c.Visit(visitor);
    return visitor.ToString();
  }
};

struct GeoJsonFeature
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(GeoJsonFeature,
                                  visitor(m_type, "type"),
                                  visitor(m_geometry, "geometry"),
                                  visitor(m_properties, "properties"))

  bool operator==(GeoJsonFeature const & data) const
  {
    return m_type == data.m_type && m_properties == data.m_properties;
  }

  bool operator!=(GeoJsonFeature const & data) const
  {
    return !operator==(data);
  }

  std::string m_type = "Feature";
  GeoJsonGeometry m_geometry;
  std::map<std::string, std::string> m_properties;
};


struct GeoJsonData
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(GeoJsonData,
                                  visitor(m_type, "type"),
                                  visitor(m_features, "features"))

  bool operator==(GeoJsonData const & data) const
  {
    return m_type == data.m_type;
  }

  bool operator!=(GeoJsonData const & data) const
  {
    return !operator==(data);
  }

  std::string m_type = "FeatureCollection";
  std::vector<GeoJsonFeature> m_features;
};


// Writer and reader
class GeojsonWriter
{
public:
  DECLARE_EXCEPTION(WriteGeojsonException, RootException);

  explicit GeojsonWriter(Writer & writer)
    : m_writer(writer)
  {}

  void Write(FileData const & fileData);

private:
  Writer & m_writer;
};

class GeojsonParser
{
public:
  explicit GeojsonParser(FileData & data): m_fileData(data) {};

  template <typename ReaderType>
  void Parse(ReaderType const & reader)
  {
    geojson::GeoJsonData data;
    NonOwningReaderSource source(reader);
    coding::DeserializerJson des(source);
    des(data);

    // Copy bookmarks from parsed 'data' into m_fileData.
    //TODO

    // Copy tracks from parsed 'data' into m_fileData.
    //TODO
  }

private:
  FileData & m_fileData;
};

}  // namespace geojson


/*class DeserializerGeojson
{
public:
  DECLARE_EXCEPTION(DeserializeException, RootException);

  explicit DeserializerGeojson(FileData & fileData);

  template <typename ReaderType>
  void Deserialize(ReaderType const & reader)
  {
    //NonOwningReaderSource src(reader);

    //geojson::GeojsonParser parser(m_fileData);

    geojson::GeoJsonData data;
    NonOwningReaderSource source(reader);
    coding::DeserializerJson des(source);
    des(data);

    //MYTHROW(DeserializeException, ("Could not parse GPX."));
  }

private:
  FileData & m_fileData;
};*/

}  // namespace kml
