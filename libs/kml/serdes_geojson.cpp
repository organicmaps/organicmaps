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

std::string DebugPrint(JsonTMap const & p)
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
}

bool GeojsonParser::Parse(std::string_view json_content)
{
  geojson::GeoJsonData geoJsonData;

  constexpr glz::opts opts{.comments = true, .error_on_unknown_keys = false, .error_on_missing_keys = false};
  auto ec = glz::read<opts>(geoJsonData, json_content);

  if (ec)
  {
    std::string err = glz::format_error(ec, json_content);
    LOG(LWARNING, ("Error parsing GeoJson:", err));
    return false;
  }

  auto getStringFromJsonMap = [](JsonTMap const & propsJson,
                                 std::string const & fieldName) -> std::optional<std::string>
  {
    if (auto const field = propsJson.find(fieldName); field != propsJson.end() && field->second.is_string())
      return field->second.get_string();
    return {};
  };

  // Copy bookmarks from parsed geoJsonData into m_fileData.
  for (auto & feature : geoJsonData.features)
  {
    if (auto const * point = std::get_if<GeoJsonGeometryPoint>(&feature.geometry))
    {
      double const longitude = point->coordinates.at(0);
      double const latitude = point->coordinates.at(1);

      auto const & propsJson = feature.properties;
      BookmarkData bookmark;
      bookmark.m_color = ColorData{.m_predefinedColor = PredefinedColor::Red};

      // Parse "name" or "label"
      if (auto const name = getStringFromJsonMap(propsJson, "name"))
        kml::SetDefaultStr(bookmark.m_name, *name);
      else if (auto const label = getStringFromJsonMap(propsJson, "label"))
        kml::SetDefaultStr(bookmark.m_name, *label);

      // Parse description
      if (auto const descr = getStringFromJsonMap(propsJson, "description"))
        kml::SetDefaultStr(bookmark.m_description, *descr);

      // Parse color
      if (auto const markerColor = getStringFromJsonMap(propsJson, "marker-color"))
      {
        auto colorRGBA = ParseHexOsmGarminColor(*markerColor);
        if (colorRGBA)
          bookmark.m_color = ColorData{.m_predefinedColor = MapPredefinedColor(*colorRGBA), .m_rgba = *colorRGBA};
      }

      // Parse icon
      // if (auto const markerSymbol = getStringFromJsonMap(propsJson, "marker-symbol"))
      //    bookmark.m_icon = ???;
      //}

      // UMap custom properties
      if (auto const umapOptions = propsJson.find("_umap_options");
          umapOptions != propsJson.end() && umapOptions->second.is_object())
      {
        JsonTMap const umap_options = umapOptions->second.get_object();
        // Parse color from properties['_umap_options']['color']
        if (auto const color = getStringFromJsonMap(umap_options, "color"))
        {
          auto colorRGBA = ParseHexOsmGarminColor(*color);
          if (colorRGBA)
            bookmark.m_color = ColorData{.m_rgba = *colorRGBA};
        }

        // TODO: Store '_umap_options' as a JSON string in some bookmark field.
      }

      bookmark.m_point = mercator::FromLatLon(latitude, longitude);
      m_fileData.m_bookmarksData.push_back(bookmark);
    }
    else if (auto const * unknownGeometry = std::get_if<GeoJsonGeometryUnknown>(&feature.geometry))
    {
      LOG(LWARNING, ("GeoJson contains unsupported geometry type:", unknownGeometry->type));
    }
  }

  // Copy tracks from parsed geoJsonData into m_fileData.
  for (auto & feature : geoJsonData.features)
  {
    if (auto const * lineGeometry = std::get_if<GeoJsonGeometryLine>(&feature.geometry))
    {
      std::vector<std::vector<double>> lineCoords = lineGeometry->coordinates;

      // Convert GeoJson properties to KML properties
      auto const & props_json = feature.properties;
      TrackData track;

      // Parse "name" or "label"
      if (auto const name = getStringFromJsonMap(props_json, "name"))
        kml::SetDefaultStr(track.m_name, *name);
      else if (auto const label = getStringFromJsonMap(props_json, "label"))
        kml::SetDefaultStr(track.m_name, *label);

      // Parse color
      std::unique_ptr<ColorData> lineColor;
      if (auto const stroke = getStringFromJsonMap(props_json, "stroke"))
      {
        auto const colorRGBA = ParseHexOsmGarminColor(*stroke);
        if (colorRGBA)
          lineColor = std::make_unique<ColorData>(PredefinedColor::None, *colorRGBA);
      }

      // UMap custom properties
      if (auto const umapOptions = props_json.find("_umap_options");
          umapOptions != props_json.end() && umapOptions->second.is_object())
      {
        JsonTMap umap_options = umapOptions->second.get_object();
        // Parse color from properties['_umap_options']['color']
        if (auto const color = getStringFromJsonMap(umap_options, "color"))
        {
          auto colorRGBA = ParseHexOsmGarminColor(*color);
          if (colorRGBA)
            lineColor = std::make_unique<ColorData>(PredefinedColor::None, *colorRGBA);
        }

        // TODO: Store '_umap_options' as a JSON string in some bookmark field.
      }

      if (lineColor)
        track.m_layers.push_back(TrackLayer{.m_color = *lineColor});

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

      track.m_geometry.m_lines.push_back(std::move(points));
      track.m_geometry.AddTimestamps({});
      m_fileData.m_tracksData.push_back(track);
    }
  }

  return true;
}

}  // namespace geojson

void DeserializerGeoJson::Deserialize(std::string_view content)
{
  ASSERT(!content.empty(), ());
  geojson::GeojsonParser parser(m_fileData);
  if (!parser.Parse(content))
  {
    // Print corrupted GeoJson file for debug and restore purposes.
    if (content[0] == '{')
      LOG(LWARNING, (content));
    MYTHROW(DeserializeException, ("Could not parse GeoJson."));
  }
}

}  // namespace kml
