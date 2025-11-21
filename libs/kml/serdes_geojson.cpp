#include "kml/serdes_geojson.hpp"
#include "kml/color_parser.hpp"

#include "ge0/geo_url_parser.hpp"
#include "geometry/mercator.hpp"

#include <map>
#include <string>

namespace kml
{
namespace geojson
{

std::string DebugPrint(GeoJsonGeometry const & g)
{
  if (auto const * point = std::get_if<GeoJsonGeometryPoint>(&g))
    return DebugPrint(*point);
  else if (auto const * line = std::get_if<GeoJsonGeometryLine>(&g))
    return DebugPrint(*line);
  else
  {
    auto const geoUnknown = std::get_if<GeoJsonGeometryUnknown>(&g);
    return "GeoJsonGeometryUnknown [type = " + geoUnknown->type + "]";
  }
}

std::string DebugPrint(JsonTMap const & p)
{
  std::ostringstream out;
  bool isFirst = true;
  out << "{";
  for (auto const & [key, value] : p)
  {
    // Add seperator if needed
    if (isFirst)
      isFirst = false;
    else
      out << ", ";

    out << '"' << key << "\" = " << DebugPrint(value) << ", ";
  }
  return out.str();
}

std::string DebugPrint(glz::json_t const & json)
{
  std::string buffer;
  if (glz::write_json(json, buffer))
    return buffer;
  else
    return "<JSON_ERROR>";
}

bool GeojsonParser::Parse(std::string_view jsonContent)
{
  geojson::GeoJsonData geoJsonData;

  glz::opts constexpr opts{.comments = true, .error_on_unknown_keys = false, .error_on_missing_keys = false};
  auto const ec = glz::read<opts>(geoJsonData, jsonContent);

  if (ec)
  {
    std::string err = glz::format_error(ec, jsonContent);
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
  for (auto const & feature : geoJsonData.features)
  {
    if (auto const * point = std::get_if<GeoJsonGeometryPoint>(&feature.geometry))
    {
      double longitude = point->coordinates[0];
      double latitude = point->coordinates[1];

      auto const & propsJson = feature.properties;
      BookmarkData bookmark;
      bookmark.m_color = ColorData{.m_predefinedColor = PredefinedColor::Red};

      std::string bookmark_name = {};

      // Parse "name" or "label"
      if (auto const gmapLocation = propsJson.find("location");
          gmapLocation != propsJson.end() && gmapLocation->second.is_object())
      {
        JsonTMap const gmapLocation_object = gmapLocation->second.get_object();
        /* Parse "location" structure from Google Maps GeoJson:

          "location": {
            "address": "Riverside Building, County Hall, Westminster Bridge Rd, London SE1 7PB, United Kingdom",
            "country_code": "GB",
            "name": "London Eye"
          }
        */
        auto const name = getStringFromJsonMap(gmapLocation_object, "name");
        if (name)
          bookmark_name = *name;
      }
      else if (auto const name = getStringFromJsonMap(propsJson, "name"))
        bookmark_name = *name;
      else if (auto const label = getStringFromJsonMap(propsJson, "label"))
        bookmark_name = *label;

      // Parse Google Maps URL if present:
      auto google_maps_url = getStringFromJsonMap(propsJson, "google_maps_url");
      if (google_maps_url)
      {
        if (google_maps_url->starts_with("http:"))
          google_maps_url->insert(4, 1, 's');  // Replace http:// with https://
        geo::UnifiedParser parser;
        geo::GeoURLInfo info;

        if (parser.Parse(*google_maps_url, info))
        {
          latitude = info.m_lat;
          longitude = info.m_lon;
        }
      }

      // Parse description
      if (auto const descr = getStringFromJsonMap(propsJson, "description"))
        kml::SetDefaultStr(bookmark.m_description, *descr);
      else if (google_maps_url)
      {
        if (bookmark_name.empty())
          kml::SetDefaultStr(bookmark.m_description, *google_maps_url);
        else
          kml::SetDefaultStr(bookmark.m_description,
                             "<a href=\"" + (*google_maps_url) + "\">" + bookmark_name + "</a>");
      }

      // Parse color
      if (auto const markerColor = getStringFromJsonMap(propsJson, "marker-color"))
      {
        auto const colorRGBA = ParseHexOsmGarminColor(*markerColor);
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
          auto const colorRGBA = ParseHexOsmGarminColor(*color);
          if (colorRGBA)
            bookmark.m_color = ColorData{.m_rgba = *colorRGBA};
        }

        // Store '_umap_options' as a JSON string to preserve all UMap properties
        if (std::string umapOptionsStr; glz::write_json(umapOptions->second, umapOptionsStr) == glz::error_code::none)
          bookmark.m_properties["_umap_options"] = umapOptionsStr;
      }

      if (!bookmark_name.empty())
        kml::SetDefaultStr(bookmark.m_name, bookmark_name);
      bookmark.m_point = mercator::FromLatLon(latitude, longitude);
      m_fileData.m_bookmarksData.push_back(bookmark);
    }
    else if (auto const * unknownGeometry = std::get_if<GeoJsonGeometryUnknown>(&feature.geometry))
    {
      LOG(LWARNING, ("GeoJson contains unsupported geometry type:", unknownGeometry->type));
    }
  }

  // Copy tracks from parsed geoJsonData into m_fileData.
  for (auto const & feature : geoJsonData.features)
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
        JsonTMap const umap_options = umapOptions->second.get_object();
        // Parse color from properties['_umap_options']['color']
        if (auto const color = getStringFromJsonMap(umap_options, "color"))
        {
          auto const colorRGBA = ParseHexOsmGarminColor(*color);
          if (colorRGBA)
            lineColor = std::make_unique<ColorData>(PredefinedColor::None, *colorRGBA);
        }

        // Store '_umap_options' as a JSON string to preserve all UMap properties
        if (std::string umapOptionsStr; glz::write_json(umapOptions->second, umapOptionsStr) == glz::error_code::none)
          track.m_properties["_umap_options"] = umapOptionsStr;
      }

      if (lineColor)
        track.m_layers.push_back(TrackLayer{.m_color = *lineColor});

      // Copy coordinates
      std::vector<geometry::PointWithAltitude> points;
      points.resize(lineCoords.size());
      for (size_t i = 0; i < lineCoords.size(); i++)
      {
        auto const & pointCoords = lineCoords[i];
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
