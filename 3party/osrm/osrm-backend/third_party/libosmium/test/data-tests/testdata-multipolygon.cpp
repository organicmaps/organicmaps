
#include <iostream>
#include <fstream>
#include <map>

#include <osmium/index/map/sparse_mem_array.hpp>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/area/problem_reporter_ogr.hpp>
#include <osmium/geom/ogr.hpp>
#include <osmium/geom/wkt.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/visitor.hpp>

typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location> index_type;

typedef osmium::handler::NodeLocationsForWays<index_type> location_handler_type;

struct less_charptr {

    bool operator()(const char* a, const char* b) const {
        return std::strcmp(a, b) < 0;
    }

}; // less_charptr

typedef std::map<const char*, const char*, less_charptr> tagmap_type;

inline tagmap_type create_map(const osmium::TagList& taglist) {
    tagmap_type map;

    for (auto& tag : taglist) {
        map[tag.key()] = tag.value();
    }

    return map;
}

class TestHandler : public osmium::handler::Handler {

    OGRDataSource* m_data_source;
    OGRLayer* m_layer_point;
    OGRLayer* m_layer_linestring;
    OGRLayer* m_layer_polygon;

    osmium::geom::OGRFactory<> m_ogr_factory;
    osmium::geom::WKTFactory<> m_wkt_factory;

    std::ofstream m_out;

    bool m_first_out {true};

public:

    TestHandler(OGRDataSource* data_source) :
        m_data_source(data_source),
        m_out("multipolygon-tests.json") {

        OGRSpatialReference sparef;
        sparef.SetWellKnownGeogCS("WGS84");

        /**************/

        m_layer_point = m_data_source->CreateLayer("points", &sparef, wkbPoint, nullptr);
        if (!m_layer_point) {
            std::cerr << "Layer creation failed.\n";
            exit(1);
        }

        OGRFieldDefn layer_point_field_id("id", OFTReal);
        layer_point_field_id.SetWidth(10);

        if (m_layer_point->CreateField(&layer_point_field_id) != OGRERR_NONE) {
            std::cerr << "Creating id field failed.\n";
            exit(1);
        }

        OGRFieldDefn layer_point_field_type("type", OFTString);
        layer_point_field_type.SetWidth(30);

        if (m_layer_point->CreateField(&layer_point_field_type) != OGRERR_NONE) {
            std::cerr << "Creating type field failed.\n";
            exit(1);
        }

        /**************/

        m_layer_linestring = m_data_source->CreateLayer("lines", &sparef, wkbLineString, nullptr);
        if (!m_layer_linestring) {
            std::cerr << "Layer creation failed.\n";
            exit(1);
        }

        OGRFieldDefn layer_linestring_field_id("id", OFTReal);
        layer_linestring_field_id.SetWidth(10);

        if (m_layer_linestring->CreateField(&layer_linestring_field_id) != OGRERR_NONE) {
            std::cerr << "Creating id field failed.\n";
            exit(1);
        }

        OGRFieldDefn layer_linestring_field_type("type", OFTString);
        layer_linestring_field_type.SetWidth(30);

        if (m_layer_linestring->CreateField(&layer_linestring_field_type) != OGRERR_NONE) {
            std::cerr << "Creating type field failed.\n";
            exit(1);
        }

        /**************/

        m_layer_polygon = m_data_source->CreateLayer("multipolygons", &sparef, wkbMultiPolygon, nullptr);
        if (!m_layer_polygon) {
            std::cerr << "Layer creation failed.\n";
            exit(1);
        }

        OGRFieldDefn layer_polygon_field_id("id", OFTInteger);
        layer_polygon_field_id.SetWidth(10);

        if (m_layer_polygon->CreateField(&layer_polygon_field_id) != OGRERR_NONE) {
            std::cerr << "Creating id field failed.\n";
            exit(1);
        }

        OGRFieldDefn layer_polygon_field_from_type("from_type", OFTString);
        layer_polygon_field_from_type.SetWidth(1);

        if (m_layer_polygon->CreateField(&layer_polygon_field_from_type) != OGRERR_NONE) {
            std::cerr << "Creating from_type field failed.\n";
            exit(1);
        }
    }

    ~TestHandler() {
        m_out << "\n]\n";
    }

    void node(const osmium::Node& node) {
        OGRFeature* feature = OGRFeature::CreateFeature(m_layer_point->GetLayerDefn());
        std::unique_ptr<OGRPoint> ogr_point = m_ogr_factory.create_point(node);
        feature->SetGeometry(ogr_point.get());
        feature->SetField("id", static_cast<double>(node.id()));
        feature->SetField("type", node.tags().get_value_by_key("type"));

        if (m_layer_point->CreateFeature(feature) != OGRERR_NONE) {
            std::cerr << "Failed to create feature.\n";
            exit(1);
        }

        OGRFeature::DestroyFeature(feature);
    }

    void way(const osmium::Way& way) {
        try {
            std::unique_ptr<OGRLineString> ogr_linestring = m_ogr_factory.create_linestring(way);
            OGRFeature* feature = OGRFeature::CreateFeature(m_layer_linestring->GetLayerDefn());
            feature->SetGeometry(ogr_linestring.get());
            feature->SetField("id", static_cast<double>(way.id()));
            feature->SetField("type", way.tags().get_value_by_key("type"));

            if (m_layer_linestring->CreateFeature(feature) != OGRERR_NONE) {
                std::cerr << "Failed to create feature.\n";
                exit(1);
            }

            OGRFeature::DestroyFeature(feature);
        } catch (osmium::geometry_error&) {
            std::cerr << "Ignoring illegal geometry for way " << way.id() << ".\n";
        }
    }

    void area(const osmium::Area& area) {
        if (m_first_out) {
            m_out << "[\n";
            m_first_out = false;
        } else {
            m_out << ",\n";
        }
        m_out << "{\n  \"test_id\": " << (area.orig_id() / 1000) << ",\n  \"area_id\": " << area.id() << ",\n  \"from_id\": " << area.orig_id() << ",\n  \"from_type\": \"" << (area.from_way() ? "way" : "relation") << "\",\n  \"wkt\": \"";
        try {
            std::string wkt = m_wkt_factory.create_multipolygon(area);
            m_out << wkt << "\",\n  \"tags\": {";

            auto tagmap = create_map(area.tags());
            bool first = true;
            for (auto& tag : tagmap) {
                if (first) {
                    first = false;
                } else {
                    m_out << ", ";
                }
                m_out << '"' << tag.first << "\": \"" << tag.second << '"';
            }
            m_out << "}\n}";
        } catch (osmium::geometry_error&) {
            m_out << "INVALID\"\n}";
        }
        try {
            std::unique_ptr<OGRMultiPolygon> ogr_polygon = m_ogr_factory.create_multipolygon(area);
            OGRFeature* feature = OGRFeature::CreateFeature(m_layer_polygon->GetLayerDefn());
            feature->SetGeometry(ogr_polygon.get());
            feature->SetField("id", static_cast<int>(area.orig_id()));

            std::string from_type;
            if (area.from_way()) {
                from_type = "w";
            } else {
                from_type = "r";
            }
            feature->SetField("from_type", from_type.c_str());

            if (m_layer_polygon->CreateFeature(feature) != OGRERR_NONE) {
                std::cerr << "Failed to create feature.\n";
                exit(1);
            }

            OGRFeature::DestroyFeature(feature);
        } catch (osmium::geometry_error&) {
            std::cerr << "Ignoring illegal geometry for area " << area.id() << " created from " << (area.from_way() ? "way" : "relation") << " with id=" << area.orig_id() << ".\n";
        }
    }

}; // class TestHandler

/* ================================================== */

OGRDataSource* initialize_database(const std::string& output_format, const std::string& output_filename) {
    OGRRegisterAll();

    OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(output_format.c_str());
    if (!driver) {
        std::cerr << output_format << " driver not available.\n";
        exit(1);
    }

    CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "FALSE");
    const char* options[] = { "SPATIALITE=TRUE", nullptr };
    OGRDataSource* data_source = driver->CreateDataSource(output_filename.c_str(), const_cast<char**>(options));
    if (!data_source) {
        std::cerr << "Creation of output file failed.\n";
        exit(1);
    }

    return data_source;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " INFILE\n";
        exit(1);
    }

    std::string output_format("SQLite");
    std::string input_filename(argv[1]);
    std::string output_filename("multipolygon.db");

    OGRDataSource* data_source = initialize_database(output_format, output_filename);

    osmium::area::ProblemReporterOGR problem_reporter(data_source);
    osmium::area::Assembler::config_type assembler_config(&problem_reporter);
    assembler_config.enable_debug_output();
    osmium::area::MultipolygonCollector<osmium::area::Assembler> collector(assembler_config);

    std::cerr << "Pass 1...\n";
    osmium::io::Reader reader1(input_filename);
    collector.read_relations(reader1);
    reader1.close();
    std::cerr << "Pass 1 done\n";

    index_type index;
    location_handler_type location_handler(index);
    location_handler.ignore_errors();

    TestHandler test_handler(data_source);

    std::cerr << "Pass 2...\n";
    osmium::io::Reader reader2(input_filename);
    osmium::apply(reader2, location_handler, test_handler, collector.handler([&test_handler](const osmium::memory::Buffer& area_buffer) {
        osmium::apply(area_buffer, test_handler);
    }));
    reader2.close();
    std::cerr << "Pass 2 done\n";

    OGRDataSource::DestroyDataSource(data_source);
    OGRCleanupAll();
}

