#include "kml/serdes_geojson.hpp"

//#include "coding/serdes_json.hpp"

//#include "base/visitor.hpp"

//#include "coding/file_reader.hpp"
//#include "coding/file_writer.hpp"

#include <map>
#include <string>
//#include <unordered_set>

namespace kml
{
namespace geojson
{


/*void GeojsonWriter::Write(FileData const & fileData)
{
  GeoJsonData data;
  coding::SerializerJson<Writer> ser(m_writer);
  ser(data);
}*/

bool GeoJsonFeature::isPoint() {
    return this->geometry.type == "Point";
}

bool GeoJsonFeature::isLine() {
    return this->geometry.type == "LineString";
}


/*template <typename ReaderType>
void GeojsonParser::Parse(ReaderType const & reader)
{
  geojson::GeoJsonData data;
  NonOwningReaderSource source(reader);
  coding::DeserializerJson des(source);
  des(data);

  // Copy bookmarks from parsed 'data' into m_fileData.
  //TODO

  // Copy tracks from parsed 'data' into m_fileData.
  //TODO
}*/

bool GeojsonParser::Parse(std::string_view & json_content) {
    geojson::GeoJsonData geoJsonData;

    constexpr glz::opts opts { .error_on_missing_keys = false };
    auto ec = glz::read<opts>(geoJsonData, json_content);

    if (ec) {
        std::string err = glz::format_error(ec, json_content);
        //std::cerr << "Parse error: " << err << "\n";
        LOG(LWARNING, ("Error parsing JSON:", err));
        return false;
    }

    // Copy bookmarks from parsed geoJsonData into m_fileData.
    for(auto feature : geoJsonData.features) {
        if (feature.isPoint())
        {
            std::vector<double> latLon = std::get<std::vector<double>>(feature.geometry.coordinates);
            glz::json_t *props_json = &feature.properties;
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
                auto colorRGBA = ParseColor((*props_json)["description"].get_string());
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

            bookmark.m_point = mercator::FromLatLon(latLon[1], latLon[0]);
            m_fileData.m_bookmarksData.push_back(bookmark);
        }
    }

    // Copy tracks from parsed geoJsonData into m_fileData.
    for(auto feature : geoJsonData.features) {
        if (feature.isLine())
        {
            std::vector<std::vector<double>> lineCoords = std::get<std::vector<std::vector<double>>>(feature.geometry.coordinates);
            glz::json_t *props_json = &feature.properties;
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
            if (props_json->contains("marker-color") && (*props_json)["marker-color"].is_string()) {
                auto colorRGBA = ParseColor((*props_json)["description"].get_string());
                if (colorRGBA) {
                    track.m_layers.push_back(TrackLayer{.m_color = ColorData{.m_rgba = *colorRGBA}});
                }
            }

            // Copy coordinates
            std::vector<m2::PointD> points;
            points.resize(lineCoords.size());
            for(size_t i=0; i<lineCoords.size(); i++) {
                auto pairCoords = lineCoords[i];
                points.at(i) = mercator::FromLatLon(pairCoords[1], pairCoords[0]);
            }

            track.m_geometry.AddLine(points);
            m_fileData.m_tracksData.push_back(track);
        }
    }

    return true;
}

}  // namespace geojson

void DeserializerGeoJson::Deserialize(std::string_view & content)
{
    //NonOwningReaderSource src(reader);

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
