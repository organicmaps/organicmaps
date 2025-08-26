#include "kml/serdes_geojson.hpp"
#include "kml/color_parser.hpp"

#include "geometry/mercator.hpp"

#include <map>
#include <string>

namespace kml
{
namespace geojson
{

bool GeoJsonFeature::isPoint() {
    return this->geometry.type == "Point";
}

bool GeoJsonFeature::isLine() {
    return this->geometry.type == "LineString";
}

bool GeojsonParser::Parse(std::string_view & json_content) {
    geojson::GeoJsonData geoJsonData;

    constexpr glz::opts opts {
      .comments = true,
      .error_on_unknown_keys = false,
      .error_on_missing_keys = false
    };
    auto ec = glz::read<opts>(geoJsonData, json_content);

    if (ec) {
        std::string err = glz::format_error(ec, json_content);
        LOG(LWARNING, ("Error parsing JSON:", err));
        return false;
    }

    // Copy bookmarks from parsed geoJsonData into m_fileData.
    for(auto feature : geoJsonData.features) {
        if (feature.isPoint())
        {
            double longitude = feature.geometry.coordinates.at(0).get_number();
            double latitude  = feature.geometry.coordinates.at(1).get_number();

            std::map<std::string, glz::json_t> *props_json = &feature.properties;
            BookmarkData bookmark;

            // Parse label
            if (props_json->contains("name") && (*props_json)["name"].is_string()) {
                auto name = kml::LocalizableString();
                kml::SetDefaultStr(name, (*props_json)["name"].get_string());
                bookmark.m_name = name;
            }
            else if (props_json->contains("label") && (*props_json)["label"].is_string()) {
                auto name = kml::LocalizableString();
                kml::SetDefaultStr(name, (*props_json)["label"].get_string());
                bookmark.m_name = name;
            }

            // Parse description
            if (props_json->contains("description") && (*props_json)["description"].is_string()) {
                auto descr = kml::LocalizableString();
                kml::SetDefaultStr(descr, (*props_json)["description"].get_string());
                bookmark.m_description = descr;
            }

            // Parse color
            if (props_json->contains("marker-color") && (*props_json)["marker-color"].is_string()) {
                auto colorRGBA = ParseColor((*props_json)["marker-color"].get_string());
                if (colorRGBA) {
                    bookmark.m_color = ColorData{.m_rgba = *colorRGBA};
                }
            }

            // Parse icon
            //if (props_json->contains("marker-symbol") && (*props_json)["marker-symbol"].is_string()) {
            //    auto const markerSymbol = (*props_json)["marker-symbol"].get<std::string>()
            //    bookmark.m_icon = ???;
            //}

            // UMap custom properties
            if (props_json->contains("_umap_options") && (*props_json)["_umap_options"].is_object()) {
                glz::json_t::object_t umap_options = (*props_json)["_umap_options"].get_object();
                // Parse color from properties['_umap_options']['color']
                if (umap_options.contains("color") && umap_options["color"].is_string()) {
                    auto colorRGBA = ParseColor(umap_options["color"].get_string());
                    if (colorRGBA) {
                        bookmark.m_color = ColorData{.m_rgba = *colorRGBA};
                    }
                }

                // TODO: Store 'umap_options' as a JSON string in some bookmark field.
            }

            bookmark.m_point = mercator::FromLatLon(latitude, longitude);
            m_fileData.m_bookmarksData.push_back(bookmark);
        }
    }

    // Copy tracks from parsed geoJsonData into m_fileData.
    for(auto feature : geoJsonData.features) {
        if (feature.isLine())
        {
            auto rawCoordinates = feature.geometry.coordinates;
            std::vector<std::vector<double>> lineCoords;

            lineCoords.resize(rawCoordinates.size());

            // Convert 'rawCoordinates' from json_t to vector<double> with type checks
            std::transform(rawCoordinates.begin(), rawCoordinates.end(), lineCoords.begin(),
                           [](const glz::json_t& json)
            {
                std::vector<double> result;
                if (json.is_array()) {
                    for(glz::json_t json_element: json.get_array()) {
                        if (json_element.is_number())
                            result.push_back(json_element.get_number());
                    }
                }

                return result;
            });

            // Convert GeoJson properties to KML properties
            std::map<std::string, glz::json_t> *props_json = &feature.properties;
            TrackData track;

            // Parse label
            if (props_json->contains("name") && (*props_json)["name"].is_string()) {
                auto name = kml::LocalizableString();
                kml::SetDefaultStr(name, (*props_json)["name"].get_string());
                track.m_name = name;
            }
            else if (props_json->contains("label") && (*props_json)["label"].is_string()) {
                auto name = kml::LocalizableString();
                kml::SetDefaultStr(name, (*props_json)["label"].get_string());
                track.m_name = name;
            }

            // Parse color
            ColorData *lineColor = nullptr;
            if (props_json->contains("stroke") && (*props_json)["stroke"].is_string()) {
                auto colorRGBA = ParseColor((*props_json)["stroke"].get_string());
                if (colorRGBA) {
                    //track.m_layers.push_back(TrackLayer{.m_color = ColorData{.m_rgba = *colorRGBA}});
                    lineColor = new ColorData{.m_rgba = *colorRGBA};
                }
            }

            // UMap custom properties
            if (props_json->contains("_umap_options") && (*props_json)["_umap_options"].is_object()) {
                glz::json_t::object_t umap_options = (*props_json)["_umap_options"].get_object();
                // Parse color from properties['_umap_options']['color']
                if (umap_options.contains("color") && umap_options["color"].is_string()) {
                    auto colorRGBA = ParseColor(umap_options["color"].get_string());
                    if (colorRGBA) {
                        lineColor = new ColorData{.m_rgba = *colorRGBA};
                    }
                }

                // TODO: Store 'umap_options' as a JSON string in some bookmark field.
            }

            if (lineColor != nullptr) {
                track.m_layers.push_back(TrackLayer{.m_color = *lineColor});
                delete lineColor;
            }


            // Copy coordinates
            std::vector<m2::PointD> points;
            points.resize(lineCoords.size());
            for(size_t i=0; i<lineCoords.size(); i++) {
                auto pairCoords = lineCoords[i];
                points.at(i) = mercator::FromLatLon(pairCoords[1], pairCoords[0]);
            }

            track.m_geometry.AddLine(points);
            track.m_geometry.AddTimestamps({});
            m_fileData.m_tracksData.push_back(track);
        }
    }

    return true;
}

}  // namespace geojson

void DeserializerGeoJson::Deserialize(std::string_view & content)
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
