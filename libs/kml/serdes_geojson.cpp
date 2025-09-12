#include "kml/serdes_geojson.hpp"
#include "kml/color_parser.hpp"

#include "geometry/mercator.hpp"

#include <map>
#include <string>

namespace kml
{
namespace geojson
{

std::string DebugPrint(GeoJsonGeometry const & g)
{
  if (std::holds_alternative<GeoJsonGeometryPoint>(g))
  {
    auto geoPoint = std::get_if<GeoJsonGeometryPoint>(&g);
    return DebugPrint(*geoPoint);
  }
  else if (std::holds_alternative<GeoJsonGeometryLine>(g))
  {
    auto geoLine = std::get_if<GeoJsonGeometryLine>(&g);
    return DebugPrint(*geoLine);
  }
  else
  {
    auto geoUnknown = std::get_if<GeoJsonGeometryUnknown>(&g);
    return "GeoJsonGeometryUnknown [type = " + geoUnknown->type + "]";
  }
}

std::string DebugPrint(std::map<std::string, glz::json_t> const & p)
{
  std::ostringstream out;
  bool isFirst = true;
  out << "{";
  for (auto const & pair : p)
  {
    // Add seperator if needed
    if (isFirst)
      isFirst = false;
    else
      out << ", ";

    out << '"' << pair.first << "\" = " << DebugPrint(pair.second) << ", ";
  }
  return out.str();
}

std::string DebugPrint(glz::json_t const & json)
{
  std::string buffer{};
  if (glz::write_json(json, buffer))
    return buffer;
  else
    return "<JSON_ERROR>";
  // return glz::write_json(json).value_or("<JSON_ERROR>");
}

bool GeoJsonFeature::isPoint() const
{
  return std::holds_alternative<GeoJsonGeometryPoint>(geometry);
}

bool GeoJsonFeature::isLine() const
{
  return std::holds_alternative<GeoJsonGeometryLine>(geometry);
}

bool GeoJsonFeature::isUnknown() const
{
  return std::holds_alternative<GeoJsonGeometryUnknown>(geometry);
}

bool GeojsonParser::Parse(std::string_view & json_content)
{
  geojson::GeoJsonData geoJsonData;

  constexpr glz::opts opts{.comments = true, .error_on_unknown_keys = false, .error_on_missing_keys = false};
  auto ec = glz::read<opts>(geoJsonData, json_content);

  if (ec)
  {
    std::string err = glz::format_error(ec, json_content);
    LOG(LWARNING, ("Error parsing JSON:", err));
    return false;
  }

  // Copy bookmarks from parsed geoJsonData into m_fileData.
  for (auto & feature : geoJsonData.features)
  {
    if (feature.isPoint())
    {
      GeoJsonGeometryPoint const * point = std::get_if<GeoJsonGeometryPoint>(&feature.geometry);
      double longitude = point->coordinates.at(0);
      double latitude = point->coordinates.at(1);

      auto const & props_json = feature.properties;
      BookmarkData bookmark;

      // Parse "name" or "label"
      if (auto const name = props_json.find("name"); name != props_json.end() && name->second.is_string())
        kml::SetDefaultStr(bookmark.m_name, name->second.get_string());
      else if (auto const label = props_json.find("label"); label != props_json.end() && label->second.is_string())
        kml::SetDefaultStr(bookmark.m_name, label->second.get_string());

      // Parse description
      if (auto const descr = props_json.find("description"); descr != props_json.end() && descr->second.is_string())
        kml::SetDefaultStr(bookmark.m_description, descr->second.get_string());

      // Parse color
      if (auto const markerColor = props_json.find("marker-color");
          markerColor != props_json.end() && markerColor->second.is_string())
      {
        auto colorRGBA = ParseHexOsmGarminColor(markerColor->second.get_string());
        if (colorRGBA)
          bookmark.m_color = ColorData{.m_predefinedColor = MapPredefinedColor(*colorRGBA), .m_rgba = *colorRGBA};
      }

      // Parse icon
      // if (props_json->contains("marker-symbol") && (*props_json)["marker-symbol"].is_string()) {
      //    auto const markerSymbol = (*props_json)["marker-symbol"].get<std::string>()
      //    bookmark.m_icon = ???;
      //}

      // UMap custom properties
      if (auto const umapOptions = props_json.find("_umap_options");
          umapOptions != props_json.end() && umapOptions->second.is_object())
      {
        glz::json_t::object_t umap_options = umapOptions->second.get_object();
        // Parse color from properties['_umap_options']['color']
        if (auto const color = umap_options.find("color"); color != umap_options.end() && color->second.is_object())
        {
          auto colorRGBA = ParseHexOsmGarminColor(color->second.get_string());
          if (colorRGBA)
            bookmark.m_color = ColorData{.m_rgba = *colorRGBA};
        }

        // TODO: Store '_umap_options' as a JSON string in some bookmark field.
      }

      bookmark.m_point = mercator::FromLatLon(latitude, longitude);
      m_fileData.m_bookmarksData.push_back(bookmark);
    }
    else if (feature.isUnknown())
    {
      GeoJsonGeometryUnknown const * unknownGeometry = std::get_if<GeoJsonGeometryUnknown>(&feature.geometry);
      LOG(LWARNING, ("GeoJson contains unsupported geometry type:", unknownGeometry->type));
    }
  }

  // Copy tracks from parsed geoJsonData into m_fileData.
  for (auto & feature : geoJsonData.features)
  {
    if (feature.isLine())
    {
      GeoJsonGeometryLine const * lineGeometry = std::get_if<GeoJsonGeometryLine>(&feature.geometry);
      std::vector<std::vector<double>> lineCoords = lineGeometry->coordinates;

      // Convert GeoJson properties to KML properties
      auto const & props_json = feature.properties;
      TrackData track;

      // Parse "name" or "label"
      if (auto const name = props_json.find("name"); name != props_json.end() && name->second.is_string())
        kml::SetDefaultStr(track.m_name, name->second.get_string());
      else if (auto const label = props_json.find("label"); label != props_json.end() && label->second.is_string())
        kml::SetDefaultStr(track.m_name, label->second.get_string());

      // Parse color
      ColorData * lineColor = nullptr;
      if (auto const stroke = props_json.find("stroke"); stroke != props_json.end() && stroke->second.is_string())
      {
        auto colorRGBA = ParseHexOsmGarminColor(stroke->second.get_string());
        if (colorRGBA)
          lineColor = new ColorData{.m_rgba = *colorRGBA};
      }

      // UMap custom properties
      if (auto const umapOptions = props_json.find("_umap_options");
          umapOptions != props_json.end() && umapOptions->second.is_object())
      {
        glz::json_t::object_t umap_options = umapOptions->second.get_object();
        // Parse color from properties['_umap_options']['color']
        if (auto const color = umap_options.find("color"); color != umap_options.end() && color->second.is_object())
        {
          auto colorRGBA = ParseHexOsmGarminColor(color->second.get_string());
          if (colorRGBA)
          {
            if (lineColor != nullptr)
              delete lineColor;
            lineColor = new ColorData{.m_rgba = *colorRGBA};
          }
        }

        // TODO: Store '_umap_options' as a JSON string in some bookmark field.
      }

      if (lineColor != nullptr)
      {
        track.m_layers.push_back(TrackLayer{.m_color = *lineColor});
        delete lineColor;
      }

      // Copy coordinates
      std::vector<geometry::PointWithAltitude> points;
      points.resize(lineCoords.size());
      for (size_t i = 0; i < lineCoords.size(); i++)
      {
        auto pointCoords = lineCoords[i];
        if (pointCoords.size() >= 3)
          // Third coordinate (if present) means altitude
          points[i] = geometry::PointWithAltitude(mercator::FromLatLon(pointCoords[1], pointCoords[0]), pointCoords[2]);
        else
          points[i] = mercator::FromLatLon(pointCoords[1], pointCoords[0]);
      }

      track.m_geometry.AddLine(points);
      track.m_geometry.AddTimestamps({});
      m_fileData.m_tracksData.push_back(track);
    }
  }

  return true;
}

}  // namespace geojson

void DeserializerGeoJson::Deserialize(std::string_view content)
{
  geojson::GeojsonParser parser(m_fileData);
  if (!parser.Parse(content))
  {
    // Print corrupted GeoJson file for debug and restore purposes.
    if (!content.empty() && content[0] == '{')
      LOG(LWARNING, (content));
    MYTHROW(DeserializeException, ("Could not parse GeoJson."));
  }
}

}  // namespace kml
