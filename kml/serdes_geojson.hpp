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
  std::string m_type;
  std::vector<m2::PointD> m_coordinates;

  template <typename Visitor>
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
      m_coordinates.at(0) = m2::PointD(coordData[0], coordData[1]);
    }

    else if (m_type == "LineString") {
      std::vector<std::vector<double>> polygonData;
      visitor(polygonData, "coordinates");
      // Copy coordinates
      m_coordinates.resize(polygonData.size());
      for(size_t i=0; i<polygonData.size(); i++) {
        auto pairCoords = polygonData[i];
        m_coordinates.at(i) = m2::PointD(pairCoords[0], pairCoords[1]);
      }
    }
    else {
      throw RootException("Unknown GeoJson geometry type", m_type);
    }
  }

  template <typename Visitor>
  void Visit(Visitor & visitor) const
  {
    // TODO!
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
                                  visitor(m_properties, std::map<std::string, std::string>(), "properties"))

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

  // Returns 'true' if there's geometry with type==Point.
  bool isPoint();

  // Returns 'true' if there's geometry with type==LineString.
  bool isLine();
};


struct GeoJsonData
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(GeoJsonData,
                                  visitor(m_type, "type"),
                                  visitor(m_features, "features"),
                                  visitor(m_properties, std::map<std::string, std::string>(), "properties"))

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
  std::map<std::string, std::string> m_properties;
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
    geojson::GeoJsonData geoJsonData;
    NonOwningReaderSource source(reader);
    coding::DeserializerJson des(source);
    des(geoJsonData);

    // Copy bookmarks from parsed geoJsonData into m_fileData.
    for(auto feature : geoJsonData.m_features) {
        if (feature.isPoint())
        {
            auto const point = feature.m_geometry.m_coordinates[0];
            BookmarkData bookmark;
            if (feature.m_properties.contains("name")) {
                auto name = kml::LocalizableString();
                kml::SetDefaultStr(name, feature.m_properties["name"]);
                bookmark.m_name = name;
            }
            if (feature.m_properties.contains("description")) {
                auto descr = kml::LocalizableString();
                kml::SetDefaultStr(descr, feature.m_properties["description"]);
                bookmark.m_description = descr;
            }
            if (feature.m_properties.contains("marker-color")) {
                //auto const markerColor = feature.m_properties["marker-color"];
                //bookmark.m_color = TODO;
            }
            if (feature.m_properties.contains("marker-symbol")) {
                //auto const markerSymbol = feature.m_properties["marker-symbol"];
                //bookmark.m_color = TODO;
            }
            bookmark.m_point = point;
            m_fileData.m_bookmarksData.push_back(bookmark);
        }
    }

    // Copy tracks from parsed geoJsonData into m_fileData.
    for(auto feature : geoJsonData.m_features) {
        if (feature.isLine())
        {
            auto const points = feature.m_geometry.m_coordinates;
            TrackData track;
            if (feature.m_properties.contains("name")) {
                auto name = kml::LocalizableString();
                kml::SetDefaultStr(name, feature.m_properties["name"]);
                track.m_name = name;
            }
            track.m_geometry.AddLine(points);
            m_fileData.m_tracksData.push_back(track);
        }
    }
  }

private:
  FileData & m_fileData;
};

}  // namespace geojson

}  // namespace kml
