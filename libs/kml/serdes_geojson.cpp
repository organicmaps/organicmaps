#include "kml/serdes_geojson.hpp"
#include "kml/color_parser.hpp"
#include "kml/serdes_common.hpp"

#include "base/math.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"
#include "coding/hex.hpp"
#include "ge0/geo_url_parser.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <string>

namespace kml
{
namespace geojson
{
std::string DebugPrint(GeoJsonGeometryPoint const & c)
{
  std::ostringstream out;
  out << "GeoJsonGeometryPoint [coordinates = " << ::DebugPrint(c.coordinates) << "]";
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
  if (auto const * multiLine = std::get_if<GeoJsonGeometryMultiLine>(&g))
    return DebugPrint(*multiLine);
  if (auto const * polygon = std::get_if<GeoJsonGeometryPolygon>(&g))
    return "GeoJsonGeometryPolygon [rings = " + std::to_string(polygon->coordinates.size()) + "]";
  if (auto const * polygons = std::get_if<GeoJsonGeometryMultiPolygon>(&g))
    return "GeoJsonGeometryMultiPolygon [parts = " + std::to_string(polygons->coordinates.size()) + "]";

  auto const * geoUnknown = std::get_if<GeoJsonGeometryUnknown>(&g);
  ASSERT(geoUnknown != nullptr, ());
  return "GeoJsonGeometryUnknown [type = " + geoUnknown->type + "]";
}

std::string DebugPrint(GenericJsonMap const & p)
{
  std::ostringstream out;
  bool isFirst = true;
  out << "{";
  for (auto const & [key, value] : p)
  {
    if (isFirst)
      isFirst = false;
    else
      out << ", ";

    out << '"' << key << "\" = " << DebugPrint(value);
  }
  out << "}";
  return out.str();
}

std::string DebugPrint(glz::generic const & json)
{
  std::string buffer;
  if (auto err = glz::write_json(json, buffer))
    return "Could not serialize to Json: " + glz::format_error(err, buffer);
  return buffer;
}

std::string DebugPrint(GeoJsonFeature const & c)
{
  std::ostringstream out;
  out << "GeoJsonFeature [type = " << c.type << ", geometry = " << DebugPrint(c.geometry)
      << ", properties = " << (c.properties ? DebugPrint(*c.properties) : "null") << "]";
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

// The timestamp array is aligned to the source positions so a rejected position also removes its time.
static kml::TrackGeometry CoordsToPoints(geojson::LineCoords const & coords, kml::MultiGeometry::TimeT & timestamps)
{
  bool const timed = !timestamps.empty();
  ASSERT(!timed || timestamps.size() == coords.size(), (timestamps.size(), coords.size()));
  kml::TrackGeometry points;
  points.reserve(coords.size());
  for (size_t i = 0; i < coords.size(); ++i)
  {
    auto const & c = coords[i];
    if (c.size() < 2 || !c[0] || !c[1] || !IsValidLatLon(*c[1], *c[0]))
    {
      LOG(LWARNING, ("Skipping invalid GeoJSON track position", i));
      continue;
    }
    if (timed)
      timestamps[points.size()] = timestamps[i];

    auto const pt = mercator::FromLatLon(*c[1], *c[0]);
    auto const altitude = c.size() >= 3 && c[2] ? ToAltitude(*c[2]) : geometry::kInvalidAltitude;
    points.emplace_back(pt, altitude);
  }

  if (timed)
    timestamps.resize(points.size());
  return points;
}

// Parses a GeoJSON timestamp value — an ISO 8601 string (with optional timezone offset or
// milliseconds, see base::StringToTimestamp) or a JSON number (Unix epoch seconds, int or float).
static time_t GeoJsonParseTimestamp(glz::generic const & value)
{
  if (value.is_number())
  {
    double const seconds = value.as<double>();
    static_assert(std::numeric_limits<time_t>::is_signed && std::numeric_limits<time_t>::radix == 2);
    // 2^digits is the exact exclusive upper bound even when time_t::max() is not representable as double.
    double const bound = std::ldexp(1.0, std::numeric_limits<time_t>::digits);
    if (seconds < -bound || seconds >= bound)
      return base::INVALID_TIME_STAMP;
    return static_cast<time_t>(seconds);
  }
  if (value.is_string())
    return base::StringToTimestamp(value.get_string());
  return base::INVALID_TIME_STAMP;
}

static kml::MultiGeometry::TimeT ParseTimestampArray(glz::generic::array_t const & array)
{
  kml::MultiGeometry::TimeT result;
  result.reserve(array.size());
  for (auto const & item : array)
    result.push_back(GeoJsonParseTimestamp(item));
  return result;
}

// Finds coord-times array in properties, checking "coordTimes", "times",
// and "coordinateProperties.times" in that order.
static glz::generic const * GetCoordTimesValue(kml::geojson::GenericJsonMap const & props)
{
  if (auto it = props.find("coordTimes"); it != props.end())
    return &it->second;
  if (auto it = props.find("times"); it != props.end())
    return &it->second;
  if (auto it = props.find("coordinateProperties"); it != props.end() && it->second.is_object())
  {
    auto const & coordProps = it->second.get_object();
    if (auto it2 = coordProps.find("times"); it2 != coordProps.end())
      return &it2->second;
  }
  return nullptr;
}

static geojson::GenericJsonMap const & GetFeatureProperties(geojson::GeoJsonFeature const & feature)
{
  static geojson::GenericJsonMap const empty;
  return feature.properties ? *feature.properties : empty;
}

// Empty components are dropped later by NormalizeAreaGeometry, like other degenerate ones.
static bool ReadAreaPolygon(geojson::PolygonCoords const & coordinates, AreaPolygon & polygon)
{
  for (size_t i = 0; i < coordinates.size(); ++i)
  {
    auto & ring = i == 0 ? polygon.m_outer : polygon.m_holes.emplace_back();
    ring.reserve(coordinates[i].size());
    for (auto const & tuple : coordinates[i])
    {
      if (tuple.size() < 2)
        return false;
      AreaPoint point{ms::LatLon(tuple[1], tuple[0]), {}};
      if (tuple.size() > 2)
        point.m_altitude = tuple[2];
      ring.push_back(point);
    }
  }
  return true;
}

static AreaStyle ReadAreaStyle(geojson::GenericJsonMap const & properties)
{
  AreaStyle style{0x55555599, 0x555555FF, 2.0, true, true};
  auto const readColor = [](geojson::GenericJsonMap const & source, char const * key, uint32_t & color)
  {
    auto const it = source.find(key);
    if (it != source.end() && it->second.is_string())
      if (auto const parsed = ParseHexOsmGarminColor(it->second.get_string()))
        color = (*parsed & 0xFFFFFF00) | (color & 0xFF);
  };
  auto const readOpacity = [](geojson::GenericJsonMap const & source, char const * key, uint32_t & color)
  {
    auto const it = source.find(key);
    if (it != source.end() && it->second.is_number())
    {
      double const opacity = it->second.get<double>();
      if (math::is_finite(opacity) && opacity >= 0 && opacity <= 1)
        color = (color & 0xFFFFFF00) | static_cast<uint32_t>(std::lround(opacity * 255));
    }
  };
  auto const readWidth = [&](geojson::GenericJsonMap const & source, char const * key)
  {
    if (auto const it = source.find(key); it != source.end() && it->second.is_number())
    {
      double const width = it->second.get<double>();
      if (math::is_finite(width) && width >= 0)
        style.m_outlineWidth = std::min(width, kMaxLineWidth);
    }
  };
  readColor(properties, "fill", style.m_fillColor);
  readColor(properties, "stroke", style.m_outlineColor);
  readOpacity(properties, "fill-opacity", style.m_fillColor);
  readOpacity(properties, "stroke-opacity", style.m_outlineColor);
  readWidth(properties, "stroke-width");
  if (auto const it = properties.find("_umap_options"); it != properties.end() && it->second.is_object())
  {
    auto const & options = it->second.get_object();
    readColor(options, "color", style.m_outlineColor);
    readColor(options, "color", style.m_fillColor);
    readColor(options, "fillColor", style.m_fillColor);
    readOpacity(options, "opacity", style.m_outlineColor);
    readOpacity(options, "fillOpacity", style.m_fillColor);
    readWidth(options, "weight");
    if (auto const fill = options.find("fill"); fill != options.end() && fill->second.is_boolean())
      style.m_fill = fill->second.get_boolean();
    if (auto const stroke = options.find("stroke"); stroke != options.end() && stroke->second.is_boolean())
      style.m_outline = stroke->second.get_boolean();
  }
  style.m_fill = style.m_fill && (style.m_fillColor & 0xFF) != 0;
  style.m_outline = style.m_outline && (style.m_outlineColor & 0xFF) != 0;
  return style;
}

struct GeoJsonRoot
{
  std::string type = "FeatureCollection";
  std::vector<glz::raw_json_view> features;
};

bool GeoJsonReader::Parse(std::string_view jsonContent)
{
  using namespace geojson;

  auto getStringFromJsonMap = [](GenericJsonMap const & propsJson,
                                 std::string const & fieldName) -> std::optional<std::string>
  {
    if (auto const field = propsJson.find(fieldName); field != propsJson.end() && field->second.is_string())
      return field->second.get_string();
    return {};
  };

  auto importFeature = [&](GeoJsonFeature const & feature)
  {
    if (auto const * point = std::get_if<GeoJsonGeometryPoint>(&feature.geometry))
    {
      if (point->coordinates.size() < 2)
      {
        LOG(LWARNING, ("Ignoring GeoJSON point with incomplete position", point->coordinates));
        return;
      }
      double longitude = point->coordinates[0];
      double latitude = point->coordinates[1];

      auto const & propsJson = GetFeatureProperties(feature);
      BookmarkData bookmark;
      bookmark.m_color = ColorData{.m_predefinedColor = PredefinedColor::Red};

      std::string bookmark_name = {};

      // Parse "name" or "label"
      if (auto const gmapLocation = propsJson.find("location");
          gmapLocation != propsJson.end() && gmapLocation->second.is_object())
      {
        auto const & gmapLocation_object = gmapLocation->second.get_object();
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

      if (!IsValidLatLon(latitude, longitude))
      {
        LOG(LWARNING, ("Skipping GeoJson point with invalid coordinates", latitude, longitude));
        return;
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

      // Parse color. A hex marker-color imports as an explicit custom color (predefined cleared,
      // forced opaque); a named color stays a preset. See NormalizeBookmarkColorData.
      if (auto const markerColor = getStringFromJsonMap(propsJson, "marker-color"))
        if (auto colorData = ParseGeoJsonColor(*markerColor))
          bookmark.m_color = NormalizeBookmarkColorData(*colorData);

      // Parse icon
      // if (auto const markerSymbol = getStringFromJsonMap(propsJson, "marker-symbol"))
      //    bookmark.m_icon = ???;
      //}

      // UMap custom properties
      if (auto const umapOptions = propsJson.find("_umap_options");
          umapOptions != propsJson.end() && umapOptions->second.is_object())
      {
        auto const & umap_options = umapOptions->second.get_object();
        // Parse color from properties['_umap_options']['color']
        if (auto const markerColor = getStringFromJsonMap(umap_options, "color"))
          if (auto colorData = ParseGeoJsonColor(*markerColor))
            bookmark.m_color = NormalizeBookmarkColorData(*colorData);

        // Store '_umap_options' as a JSON string to preserve all UMap properties
        if (std::string umapOptionsStr; glz::write_json(umapOptions->second, umapOptionsStr) == glz::error_code::none)
          bookmark.m_properties["_umap_options"] = std::move(umapOptionsStr);
      }

      if (!bookmark_name.empty())
        SetDefaultStr(bookmark.m_name, bookmark_name);
      bookmark.m_point = mercator::FromLatLon(latitude, longitude);
      m_fileData.m_bookmarksData.push_back(std::move(bookmark));
      return;
    }

    auto const * lineGeometry = std::get_if<GeoJsonGeometryLine>(&feature.geometry);
    auto const * multilineGeometry = std::get_if<GeoJsonGeometryMultiLine>(&feature.geometry);

    if (lineGeometry != nullptr || multilineGeometry != nullptr)
    {
      // Convert GeoJson properties to KML properties
      auto const & props_json = GetFeatureProperties(feature);
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
        auto const & umap_options = umapOptions->second.get_object();
        // Parse color from properties['_umap_options']['color']
        if (auto const color = getStringFromJsonMap(umap_options, "color"))
          lineColor = ParseGeoJsonColor(*color);

        // Store '_umap_options' as a JSON string to preserve all UMap properties
        if (std::string umapOptionsStr; glz::write_json(umapOptions->second, umapOptionsStr) == glz::error_code::none)
          track.m_properties["_umap_options"] = std::move(umapOptionsStr);
      }

      if (lineColor)
        track.m_layers.push_back(TrackLayer{.m_color = *lineColor});

      // Convert line(s) coordinates. coordTimes may be a flat array (LineString) or an array
      // of per-line arrays (MultiLineString); see GetCoordTimesValue for the property aliases.
      auto const * coordTimesVal = GetCoordTimesValue(props_json);
      auto appendLine = [&](LineCoords const & coords, glz::generic const * timesValue)
      {
        kml::MultiGeometry::TimeT times;
        auto const * array = timesValue && timesValue->is_array() ? &timesValue->get_array() : nullptr;
        if (array && array->size() == coords.size())
          times = ParseTimestampArray(*array);
        else if (timesValue && (!array || !array->empty()))
          LOG(LWARNING, ("Invalid GeoJSON timestamp array for track section", GetDefaultStr(track.m_name)));

        auto line = CoordsToPoints(coords, times);
        if (line.size() < 2)
          return;

        track.m_geometry.m_lines.push_back(std::move(line));
        track.m_geometry.m_timestamps.push_back(std::move(times));
      };

      if (lineGeometry)
        appendLine(lineGeometry->coordinates, coordTimesVal);
      else
      {
        auto const & lines = multilineGeometry->coordinates;
        auto const * arrays = coordTimesVal && coordTimesVal->is_array() ? &coordTimesVal->get_array() : nullptr;
        if (coordTimesVal && (!arrays || arrays->size() > lines.size()))
        {
          LOG(LWARNING, ("Invalid GeoJSON track timestamp arrays", GetDefaultStr(track.m_name)));
          arrays = nullptr;
        }
        for (size_t i = 0; i < lines.size(); ++i)
          appendLine(lines[i], arrays && i < arrays->size() ? &(*arrays)[i] : nullptr);
      }

      if (track.m_geometry.HasLines())
        m_fileData.m_tracksData.push_back(std::move(track));
      else
        LOG(LWARNING, ("GeoJson track without geometry:", GetDefaultStr(track.m_name)));
      return;
    }

    auto const * polygon = std::get_if<GeoJsonGeometryPolygon>(&feature.geometry);
    auto const * multiPolygon = std::get_if<GeoJsonGeometryMultiPolygon>(&feature.geometry);
    if (!polygon && !multiPolygon)
    {
      LOG(LWARNING,
          ("GeoJson contains unsupported geometry type:", std::get<GeoJsonGeometryUnknown>(feature.geometry).type));
      return;
    }
    AreaData area;
    area.m_isMultiPolygon = multiPolygon != nullptr;
    bool valid = true;
    if (polygon)
      valid = ReadAreaPolygon(polygon->coordinates, area.m_polygons.emplace_back());
    else
      for (auto const & part : multiPolygon->coordinates)
        if (!ReadAreaPolygon(part, area.m_polygons.emplace_back()))
        {
          valid = false;
          break;
        }
    if (!valid || !NormalizeAreaGeometry(area.m_polygons))
    {
      LOG(LWARNING, ("Ignoring invalid GeoJSON polygon"));
      return;
    }
    // Normalization can remove a closing vertex, so inspect only retained heights.
    auto const hasAltitude = [](AreaRing const & ring)
    {
      return std::any_of(ring.begin(), ring.end(),
                         [](AreaPoint const & point) { return point.m_altitude.has_value(); });
    };
    area.m_altitudeIsWgs84 = std::any_of(area.m_polygons.begin(), area.m_polygons.end(), [&](AreaPolygon const & part)
    { return hasAltitude(part.m_outer) || std::any_of(part.m_holes.begin(), part.m_holes.end(), hasAltitude); });
    auto const & properties = GetFeatureProperties(feature);
    for (char const * key : {"name", "title", "label"})
      if (auto const name = getStringFromJsonMap(properties, key); name && !name->empty())
      {
        SetDefaultStr(area.m_name, *name);
        break;
      }
    if (auto const description = getStringFromJsonMap(properties, "description"); description && !description->empty())
      SetDefaultStr(area.m_description, *description);
    if (auto const it = properties.find("visibility"); it != properties.end() && it->second.is_boolean())
      area.m_visible = it->second.get_boolean();
    if (auto const it = properties.find("time"); it != properties.end())
      if (auto const stamp = GeoJsonParseTimestamp(it->second); stamp != base::INVALID_TIME_STAMP)
        area.m_timestamp = TimestampClock::from_time_t(stamp);
    area.m_style = ReadAreaStyle(properties);
    if (auto const it = properties.find("_umap_options"); it != properties.end() && it->second.is_object())
      if (std::string json; !glz::write_json(it->second, json))
        area.m_properties["_umap_options"] = std::move(json);
    m_fileData.m_areasData.push_back(std::move(area));
  };

  glz::opts constexpr opts{.comments = true, .error_on_unknown_keys = false};
  GeoJsonRoot root;
  auto ec = glz::read<opts>(root, jsonContent);
  if (!ec)
  {
    if (root.type == "FeatureCollection")
    {
      for (auto const & json : root.features)
      {
        GeoJsonFeature feature;
        if (auto const error = glz::read<opts>(feature, json.str))
        {
          LOG(LWARNING, ("Ignoring invalid GeoJSON feature:", glz::format_error(error, json.str)));
          continue;
        }
        importFeature(feature);
      }
    }
    else
    {
      GeoJsonFeature feature;
      if (root.type == "Feature")
        ec = glz::read<opts>(feature, jsonContent);
      else
        ec = glz::read<opts>(feature.geometry, jsonContent);
      if (!ec)
        importFeature(feature);
    }
  }
  if (ec)
  {
    LOG(LWARNING, ("Error parsing GeoJson:", glz::format_error(ec, jsonContent)));
    return false;
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

static geojson::LineCoords ConvertPoints2GeoJsonCoords(TrackGeometry const & points)
{
  geojson::LineCoords coordinates;
  coordinates.reserve(points.size());
  for (auto const & point : points)
  {
    auto const [lat, lon] = mercator::ToLatLon(point);
    if (auto const altitude = point.GetAltitude(); altitude != geometry::kInvalidAltitude)
      coordinates.push_back({lon, lat, altitude});
    else
      coordinates.push_back({lon, lat});
  }
  return coordinates;
}

static glz::generic TimestampsToJsonArray(MultiGeometry::TimeT const & timestamps)
{
  glz::generic::array_t arr;
  arr.reserve(timestamps.size());
  for (auto const ts : timestamps)
    arr.emplace_back(base::TimestampToString(ts));

  glz::generic result;
  result.data = std::move(arr);
  return result;
}

struct GeoJsonOpts : glz::opts
{
  static constexpr std::string_view float_format = "{:.10g}";
};

template <typename Options>
struct GeoJsonOptsPrettify : Options
{
  static constexpr bool prettify = true;
  static constexpr auto indentation_width = 2;
};

template <typename Options>
static auto WriteJson(geojson::GeoJsonData const & data, std::string & buffer, bool minimize)
{
  constexpr Options opts{};
  constexpr GeoJsonOptsPrettify<Options> prettyOpts{};
  return minimize ? glz::write<opts>(data, buffer) : glz::write<prettyOpts>(data, buffer);
}

static geojson::PolygonCoords WriteAreaPolygon(AreaPolygon const & polygon, bool altitudeIsWgs84)
{
  geojson::PolygonCoords coordinates;
  auto const append = [&](AreaRing const & ring)
  {
    ASSERT_GREATER_OR_EQUAL(ring.size(), 3, ());
    auto & points = coordinates.emplace_back();
    points.reserve(ring.size() + 1);
    for (auto const & point : ring)
    {
      auto & tuple = points.emplace_back(std::initializer_list<double>{point.m_latLon.m_lon, point.m_latLon.m_lat});
      // GeoJSON Z uses WGS84 ellipsoid heights; KML heights use other references and are omitted.
      if (point.m_altitude && altitudeIsWgs84)
        tuple.push_back(*point.m_altitude);
    }
    points.push_back(points.front());
  };
  append(polygon.m_outer);
  for (auto const & hole : polygon.m_holes)
    append(hole);
  return coordinates;
}

void GeoJsonWriter::Write(FileData const & fileData, bool minimizeOutput)
{
  using namespace geojson;

  // Convert FileData to GeoJsonData and then let Glaze generate Json from it.
  std::vector<GeoJsonFeature> geoJsonFeatures;

  // Convert Bookmarks
  for (BookmarkData const & bookmark : fileData.m_bookmarksData)
  {
    auto const [lat, lon] = mercator::ToLatLon(bookmark.m_point);
    GenericJsonMap bookmarkProperties;
    if (auto name = GetPreferredBookmarkName(bookmark, "default"); !name.empty())
      bookmarkProperties["name"] = std::move(name);
    bookmarkProperties["marker-color"] = ToGeoJsonColor(bookmark.m_color);
    if (auto const description = GetStringForExport(bookmark.m_description); !description.empty())
      bookmarkProperties["description"] = std::string{description};

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

  // Track stops degrade to plain Point features (GeoJSON has no notion of per-track stops), so
  // reimporting yields bookmarks, not stops. KML keeps full fidelity.
  for (TrackData const & track : fileData.m_tracksData)
  {
    for (TrackStopData const & stop : track.m_stops)
    {
      auto const [lat, lon] = mercator::ToLatLon(stop.m_point);
      GenericJsonMap stopProperties;
      if (!stop.m_name.empty())
        stopProperties["name"] = stop.m_name;
      geoJsonFeatures.push_back(GeoJsonFeature{.geometry = GeoJsonGeometryPoint{.coordinates = {lon, lat}},
                                               .properties = std::move(stopProperties)});
    }
  }

  // Convert Tracks
  for (size_t i = 0; i < fileData.m_tracksData.size(); i++)
  {
    TrackData const & track = fileData.m_tracksData[i];
    auto const & geom = track.m_geometry;
    ASSERT(geom.IsValid(), ());
    bool const isMultiline = geom.m_lines.size() > 1;
    ASSERT(!track.m_layers.empty(), ());
    auto const color = track.m_layers.front().m_color;

    GenericJsonMap trackProps;
    if (auto const name = GetStringForExport(track.m_name); !name.empty())
      trackProps["name"] = std::string{name};
    trackProps["stroke"] = ToGeoJsonColor(color);
    if (auto const description = GetStringForExport(track.m_description); !description.empty())
      trackProps["description"] = std::string{description};

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
      std::vector<LineCoords> lines;
      lines.reserve(geom.m_lines.size());
      for (auto const & line : geom.m_lines)
        lines.push_back(ConvertPoints2GeoJsonCoords(line));
      trackGeometry = GeoJsonGeometryMultiLine{.coordinates = std::move(lines)};
    }
    else
    {
      trackGeometry = GeoJsonGeometryLine{.coordinates = ConvertPoints2GeoJsonCoords(geom.m_lines.front())};
    }

    auto const & allTimestamps = geom.m_timestamps;
    if (auto const firstTimed = std::ranges::find_if(allTimestamps, [](auto const & times) { return !times.empty(); });
        firstTimed != allTimestamps.end())
    {
      trackProps["time"] = base::TimestampToString(firstTimed->front());

      if (isMultiline)
      {
        glz::generic::array_t outer;
        outer.reserve(geom.m_lines.size());
        for (auto const & lineTs : allTimestamps)
          outer.push_back(TimestampsToJsonArray(lineTs));
        glz::generic coordTimes;
        coordTimes.data = std::move(outer);
        trackProps["coordTimes"] = std::move(coordTimes);
      }
      else
        trackProps["coordTimes"] = TimestampsToJsonArray(allTimestamps[0]);
    }

    geoJsonFeatures.push_back(
        GeoJsonFeature{.geometry = std::move(trackGeometry), .properties = std::move(trackProps)});
  }

  for (auto const & area : fileData.m_areasData)
  {
    ASSERT(!area.m_polygons.empty(), ());
    GenericJsonMap properties;
    if (auto const name = GetStringForExport(area.m_name); !name.empty())
      properties["name"] = std::string(name);
    if (auto const description = GetStringForExport(area.m_description); !description.empty())
      properties["description"] = std::string(description);
    if (!area.m_visible)
      properties["visibility"] = false;
    properties["fill"] = "#" + NumToHex(area.m_style.m_fillColor >> 8).substr(2);
    properties["stroke"] = "#" + NumToHex(area.m_style.m_outlineColor >> 8).substr(2);
    properties["fill-opacity"] = area.m_style.m_fill ? (area.m_style.m_fillColor & 0xFF) / 255.0 : 0.0;
    properties["stroke-opacity"] = area.m_style.m_outline ? (area.m_style.m_outlineColor & 0xFF) / 255.0 : 0.0;
    properties["stroke-width"] = area.m_style.m_outlineWidth;
    if (area.m_timestamp != Timestamp{})
      properties["time"] = base::TimestampToString(TimestampClock::to_time_t(area.m_timestamp));
    if (auto const it = area.m_properties.find("_umap_options"); it != area.m_properties.end())
    {
      GenericJsonMap options;
      if (auto const error = glz::read_json(options, it->second))
        LOG(LWARNING, ("Error parsing '_umap_options' from KML properties:", glz::format_error(error, it->second)));
      else
      {
        options["color"] = properties["stroke"];
        options["fillColor"] = properties["fill"];
        options["opacity"] = (area.m_style.m_outlineColor & 0xFF) / 255.0;
        options["fillOpacity"] = (area.m_style.m_fillColor & 0xFF) / 255.0;
        options["weight"] = area.m_style.m_outlineWidth;
        options["fill"] = area.m_style.m_fill;
        options["stroke"] = area.m_style.m_outline;
        properties["_umap_options"] = std::move(options);
      }
    }
    if (area.m_isMultiPolygon)
    {
      GeoJsonGeometryMultiPolygon geometry;
      for (auto const & polygon : area.m_polygons)
        geometry.coordinates.push_back(WriteAreaPolygon(polygon, area.m_altitudeIsWgs84));
      geoJsonFeatures.push_back({.geometry = std::move(geometry), .properties = std::move(properties)});
    }
    else
    {
      // KML MultiGeometry contains independent polygons, which may overlap.
      // Separate features preserve that meaning without inventing a MultiPolygon union.
      for (auto const & polygon : area.m_polygons)
        geoJsonFeatures.push_back(
            {.geometry = GeoJsonGeometryPolygon{.coordinates = WriteAreaPolygon(polygon, area.m_altitudeIsWgs84)},
             .properties = properties});
    }
  }
  GeoJsonData const geoJsonData{.features = std::move(geoJsonFeatures), .properties = std::nullopt};

  std::string buffer;
  // Keep full-precision geographic area coordinates, including in files containing points and lines.
  auto const error = fileData.m_areasData.empty() ? WriteJson<GeoJsonOpts>(geoJsonData, buffer, minimizeOutput)
                                                  : WriteJson<glz::opts>(geoJsonData, buffer, minimizeOutput);

  if (error)
    MYTHROW(WriteGeoJsonException, ("Could not write to GeoJson: " + glz::format_error(error)));

  // Write GeoJson.
  this->m_writer << buffer;
}

}  // namespace kml
