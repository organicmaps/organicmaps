#include "kml/serdes_geojson.hpp"
#include "kml/color_parser.hpp"

#include "base/string_utils.hpp"
#include "coding/hex.hpp"
#include "ge0/geo_url_parser.hpp"
#include "geometry/mercator.hpp"

#include <map>
#include <string>

namespace kml
{
namespace geojson
{
std::string DebugPrint(GeoJsonGeometryPoint const & c)
{
  std::ostringstream out;
  out << "GeoJsonGeometryPoint [coordinates = " << c.coordinates[1] << ", " << c.coordinates[0] << "]";
  return out.str();
}

std::string DebugPrint(GeoJsonGeometryLine const & c)
{
  std::ostringstream out;
  out << "GeoJsonGeometryLine [coordinates = " << c.coordinates.size() << " point(s)]";
  return out.str();
}

std::string DebugPrint(GeoJsonGeometryMultiLine const & c)
{
  std::ostringstream out;
  out << "GeoJsonGeometryMultiLine [coordinates = " << c.coordinates.size() << " lines(s)]";
  return out.str();
}

std::string DebugPrint(GeoJsonGeometry const & g)
{
  if (auto const * point = std::get_if<GeoJsonGeometryPoint>(&g))
    return DebugPrint(*point);
  if (auto const * line = std::get_if<GeoJsonGeometryLine>(&g))
    return DebugPrint(*line);

  auto const geoUnknown = std::get_if<GeoJsonGeometryUnknown>(&g);
  return "GeoJsonGeometryUnknown [type = " + geoUnknown->type + "]";
}

std::string DebugPrint(GenericJsonMap const & p)
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

std::string DebugPrint(glz::generic const & json)
{
  std::string buffer;
  if (glz::write_json(json, buffer))
    return buffer;
  else
    return "<JSON_ERROR>";
}

std::string DebugPrint(GeoJsonFeature const & c)
{
  std::ostringstream out;
  out << "GeoJsonFeature [type = " << c.type << ", geometry = " << DebugPrint(c.geometry)
      << ", properties = " << DebugPrint(c.properties) << "]";
  return out.str();
}

std::string ToGeoJsonColor(ColorData color)
{
  // Returns string compatible with CSS color palette.
  if (color.m_predefinedColor == PredefinedColor::None)
  {
    if (color.m_rgba == 0)
      return "red";

    return "#" + NumToHex(color.m_rgba >> 8).substr(2);
  }

  switch (color.m_predefinedColor)
  {
    using enum PredefinedColor;
  case None: return {};
  case Red: return "red";
  case Pink: return "pink";
  case Purple: return "purple";
  case DeepPurple: return "rebeccapurple";
  case Blue: return "blue";
  case LightBlue: return "lightblue";
  case Cyan: return "cyan";
  case Teal: return "teal";
  case Green: return "green";
  case Lime: return "lime";
  case Yellow: return "yellow";
  case Orange: return "orange";
  case DeepOrange: return "tomato";
  case Brown: return "brown";
  case Gray: return "gray";
  case BlueGray: return "slategray";
  default: UNREACHABLE();
  }
}

std::optional<ColorData> ParseGeoJsonColor(std::string const & color)
{
  // Try to recognize color from string and update `destColor` value.
  // Input color could be hex string "#FF8000", or some color name "red", "orange".

  // Check if color matches any predefined color
  auto const predefColor = FindPredefinedColor(color);
  if (predefColor != PredefinedColor::None)
    return ColorData{.m_predefinedColor = predefColor};

  // Convert color using Hex parser, Garmin and OSM color palettes.
  if (auto const colorRGBA = ParseHexOsmGarminColor(color))
    return ColorData{.m_predefinedColor = MapPredefinedColor(*colorRGBA), .m_rgba = *colorRGBA};

  return {};
}

PredefinedColor FindPredefinedColor(std::string colorName)
{
  // Try to match color name to a name we use for GeoJson.
  strings::AsciiToLower(colorName);

  if (colorName == "red")
    return PredefinedColor::Red;
  if (colorName == "pink")
    return PredefinedColor::Pink;
  if (colorName == "purple")
    return PredefinedColor::Purple;
  if (colorName == "rebeccapurple")
    return PredefinedColor::DeepPurple;
  if (colorName == "blue")
    return PredefinedColor::Blue;
  if (colorName == "lightblue")
    return PredefinedColor::LightBlue;
  if (colorName == "cyan")
    return PredefinedColor::Cyan;
  if (colorName == "teal")
    return PredefinedColor::Teal;
  if (colorName == "green")
    return PredefinedColor::Green;
  if (colorName == "lime")
    return PredefinedColor::Lime;
  if (colorName == "yellow")
    return PredefinedColor::Yellow;
  if (colorName == "orange")
    return PredefinedColor::Orange;
  if (colorName == "tomato")
    return PredefinedColor::DeepOrange;
  if (colorName == "brown")
    return PredefinedColor::Brown;
  if (colorName == "gray")
    return PredefinedColor::Gray;
  if (colorName == "slategray")
    return PredefinedColor::BlueGray;
  return PredefinedColor::None;
}

}  // namespace geojson

std::vector<geometry::PointWithAltitude> CoordsToPoints(std::vector<std::vector<double>> const & coords)
{
  std::vector<geometry::PointWithAltitude> points;
  points.reserve(coords.size());
  for (auto const & c : coords)
  {
    auto const pt = mercator::FromLatLon(c[1], c[0]);
    if (c.size() >= 3)
      // Third coordinate (if present) means altitude
      points.emplace_back(pt, static_cast<geometry::Altitude>(std::round(c[2])));
    else
      points.emplace_back(pt);
  }

  return points;
}

bool GeoJsonReader::Parse(std::string_view jsonContent)
{
  using namespace geojson;

  GeoJsonData geoJsonData;

  glz::opts constexpr opts{.comments = true, .error_on_unknown_keys = false, .error_on_missing_keys = false};
  auto const ec = glz::read<opts>(geoJsonData, jsonContent);

  if (ec)
  {
    std::string err = glz::format_error(ec, jsonContent);
    LOG(LWARNING, ("Error parsing GeoJson:", err));
    return false;
  }

  auto getStringFromJsonMap = [](GenericJsonMap const & propsJson,
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
        GenericJsonMap const gmapLocation_object = gmapLocation->second.get_object();
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
        SetDefaultStr(bookmark.m_description, *descr);
      else if (google_maps_url)
      {
        if (bookmark_name.empty())
          SetDefaultStr(bookmark.m_description, *google_maps_url);
        else
          SetDefaultStr(bookmark.m_description, "<a href=\"" + (*google_maps_url) + "\">" + bookmark_name + "</a>");
      }

      // Parse color
      if (auto const markerColor = getStringFromJsonMap(propsJson, "marker-color"))
        if (auto colorData = ParseGeoJsonColor(*markerColor))
          bookmark.m_color = *colorData;

      // Parse icon
      // if (auto const markerSymbol = getStringFromJsonMap(propsJson, "marker-symbol"))
      //    bookmark.m_icon = ???;
      //}

      // UMap custom properties
      if (auto const umapOptions = propsJson.find("_umap_options");
          umapOptions != propsJson.end() && umapOptions->second.is_object())
      {
        GenericJsonMap const umap_options = umapOptions->second.get_object();
        // Parse color from properties['_umap_options']['color']
        if (auto const markerColor = getStringFromJsonMap(propsJson, "color"))
          if (auto colorData = ParseGeoJsonColor(*markerColor))
            bookmark.m_color = *colorData;

        // Store '_umap_options' as a JSON string to preserve all UMap properties
        if (std::string umapOptionsStr; glz::write_json(umapOptions->second, umapOptionsStr) == glz::error_code::none)
          bookmark.m_properties["_umap_options"] = umapOptionsStr;
      }

      if (!bookmark_name.empty())
        SetDefaultStr(bookmark.m_name, bookmark_name);
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
    auto const * lineGeometry = std::get_if<GeoJsonGeometryLine>(&feature.geometry);
    auto const * multilineGeometry = std::get_if<GeoJsonGeometryMultiLine>(&feature.geometry);

    if (lineGeometry != nullptr || multilineGeometry != nullptr)
    {
      // Convert GeoJson properties to KML properties
      auto const & props_json = feature.properties;
      TrackData track;

      // Parse "name" or "label"
      if (auto const name = getStringFromJsonMap(props_json, "name"))
        SetDefaultStr(track.m_name, *name);
      else if (auto const label = getStringFromJsonMap(props_json, "label"))
        SetDefaultStr(track.m_name, *label);

      // Parse color
      std::optional<ColorData> lineColor;
      if (auto const stroke = getStringFromJsonMap(props_json, "stroke"))
        lineColor = ParseGeoJsonColor(*stroke);

      // UMap custom properties
      if (auto const umapOptions = props_json.find("_umap_options");
          umapOptions != props_json.end() && umapOptions->second.is_object())
      {
        GenericJsonMap const umap_options = umapOptions->second.get_object();
        // Parse color from properties['_umap_options']['color']
        if (auto const color = getStringFromJsonMap(umap_options, "color"))
          lineColor = ParseGeoJsonColor(*color);

        // Store '_umap_options' as a JSON string to preserve all UMap properties
        if (std::string umapOptionsStr; glz::write_json(umapOptions->second, umapOptionsStr) == glz::error_code::none)
          track.m_properties["_umap_options"] = umapOptionsStr;
      }

      if (lineColor)
        track.m_layers.push_back(TrackLayer{.m_color = *lineColor});

      // Convert line(s) coordinates
      if (lineGeometry)
      {
        track.m_geometry.m_lines.push_back(CoordsToPoints(lineGeometry->coordinates));
        track.m_geometry.AddTimestamps({});  // TODO: parse timestamps from GeoJson.
      }
      if (multilineGeometry)
      {
        for (auto & coords : multilineGeometry->coordinates)
        {
          track.m_geometry.m_lines.push_back(CoordsToPoints(coords));
          track.m_geometry.AddTimestamps({});  // TODO: parse timestamps from GeoJson.
        }
      }

      m_fileData.m_tracksData.push_back(std::move(track));
    }
  }

  return true;
}

void GeoJsonReader::Deserialize(std::string_view content)
{
  ASSERT(!content.empty(), ());

  if (!this->Parse(content))
  {
    // Print corrupted GeoJson file for debug and restore purposes.
    if (content[0] == '{')
      LOG(LWARNING, (content));
    MYTHROW(DeserializeException, ("Could not parse GeoJson."));
  }
}

std::vector<std::vector<double>> ConvertPoints2GeoJsonCoords(std::vector<geometry::PointWithAltitude> const & points,
                                                             bool addAltitude = false)
{
  std::vector<std::vector<double>> coordinates;
  coordinates.reserve(points.size());
  for (auto const & point : points)
  {
    auto const [lat, lon] = mercator::ToLatLon(point);
    if (addAltitude)
      coordinates.emplace_back(std::vector{lon, lat, static_cast<double>(point.GetAltitude())});
    else
      coordinates.emplace_back(std::vector{lon, lat});
  }
  return coordinates;
}

// Options for Glaze Json writer
struct GeoJsonOpts : glz::opts
{
  static constexpr std::string_view float_format = "{:.10g}";
};

void GeoJsonWriter::Write(FileData const & fileData, bool minimize_output)
{
  using namespace geojson;

  // Convert FileData to GeoJsonData and then let Glaze generate Json from it.
  std::vector<GeoJsonFeature> geoJsonFeatures;

  // Convert Bookmarks
  for (BookmarkData const & bookmark : fileData.m_bookmarksData)
  {
    auto const [lat, lon] = mercator::ToLatLon(bookmark.m_point);
    GenericJsonMap bookmarkProperties{{"name", GetDefaultStr(bookmark.m_name)},
                                      {"marker-color", ToGeoJsonColor(bookmark.m_color)}};
    if (!bookmark.m_description.empty())
      bookmarkProperties["description"] = GetDefaultStr(bookmark.m_description);

    // Add '_umap_options' if needed.
    if (auto const umapOptionsPair = bookmark.m_properties.find("_umap_options");
        umapOptionsPair != bookmark.m_properties.end())
    {
      GenericJsonMap umap_options_obj;
      if (auto const error = glz::read_json(umap_options_obj, umapOptionsPair->second))
      {
        // Some error happened!
        std::string const err = glz::format_error(error, umapOptionsPair->second);
        LOG(LWARNING, ("Error parsing '_umap_options' from KML properties:", err));
      }
      else
      {
        // Update known UMap properties.
        umap_options_obj["color"] = ToGeoJsonColor(bookmark.m_color);
        bookmarkProperties["_umap_options"] = std::move(umap_options_obj);
      }
    }

    geoJsonFeatures.push_back(GeoJsonFeature{.geometry = GeoJsonGeometryPoint{.coordinates = {lon, lat}},
                                             .properties = std::move(bookmarkProperties)});
  }

  // Convert Tracks
  for (size_t i = 0; i < fileData.m_tracksData.size(); i++)
  {
    TrackData const & track = fileData.m_tracksData[i];
    auto const linesCount = track.m_geometry.m_lines.size();
    bool isMultiline = linesCount > 1;
    if (linesCount == 0)
      continue;

    ASSERT(!track.m_layers.empty(), ());
    auto const color = track.m_layers.front().m_color;

    GenericJsonMap trackProps{{"name", GetDefaultStr(track.m_name)}, {"stroke", ToGeoJsonColor(color)}};
    if (!track.m_description.empty())
      trackProps["description"] = GetDefaultStr(track.m_description);

    // Add '_umap_options' if needed.
    if (auto const umapOptionsPair = track.m_properties.find("_umap_options");
        umapOptionsPair != track.m_properties.end())
    {
      GenericJsonMap umap_options_obj;
      if (auto const error = glz::read_json(umap_options_obj, umapOptionsPair->second))
      {
        // Some error happened!
        std::string const err = glz::format_error(error, umapOptionsPair->second);
        LOG(LWARNING, ("Error parsing '_umap_options' from KML properties:", err));
      }
      else
      {
        // Update known UMap properties.
        umap_options_obj["color"] = ToGeoJsonColor(color);
        trackProps["_umap_options"] = std::move(umap_options_obj);
      }
    }

    GeoJsonGeometry trackGeometry;
    if (isMultiline)
    {
      std::vector<GeoJsonGeometryMultiLine::LineCoords> lines;
      for (auto const & trackLine : track.m_geometry.m_lines)
        lines.push_back(ConvertPoints2GeoJsonCoords(trackLine));  // TODO: add timestamps to GeoJson lines.

      trackGeometry = GeoJsonGeometryMultiLine{.coordinates = std::move(lines)};
    }
    else
    {
      auto const & points = track.m_geometry.m_lines[0];
      // TODO: add timestamps to GeoJson lines.
      trackGeometry = GeoJsonGeometryLine{.coordinates = ConvertPoints2GeoJsonCoords(points)};
    }

    geoJsonFeatures.push_back(
        GeoJsonFeature{.geometry = std::move(trackGeometry), .properties = std::move(trackProps)});
  }

  GeoJsonData const geoJsonData{.features = std::move(geoJsonFeatures), .properties = std::nullopt};

  // Export to GeoJson string.
  glz::error_ctx error;
  std::string buffer;

  if (minimize_output)
  {
    GeoJsonOpts constexpr opts{glz::opts{.prettify = false}};
    error = glz::write<opts>(geoJsonData, buffer);
  }
  else
  {
    GeoJsonOpts constexpr opts{glz::opts{.prettify = true, .indentation_width = 2}};
    error = glz::write<opts>(geoJsonData, buffer);
  }

  if (error)
  {
    std::string const err = glz::format_error(error, buffer);
    MYTHROW(WriteGeoJsonException, ("Could not write to GeoJson: " + err));
  }

  // Write GeoJson.
  this->m_writer << buffer;
}

}  // namespace kml
