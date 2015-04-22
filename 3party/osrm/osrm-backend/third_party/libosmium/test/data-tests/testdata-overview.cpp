/* The code in this file is released into the Public Domain. */

#include <iostream>

#include <osmium/index/map/sparse_mem_array.hpp>

#include <osmium/geom/ogr.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/visitor.hpp>

typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location> index_type;
typedef osmium::handler::NodeLocationsForWays<index_type> location_handler_type;

class TestOverviewHandler : public osmium::handler::Handler {

    OGRDataSource* m_data_source;

    OGRLayer* m_layer_nodes;
    OGRLayer* m_layer_labels;
    OGRLayer* m_layer_ways;

    osmium::geom::OGRFactory<> m_factory;

public:

    TestOverviewHandler(const std::string& driver_name, const std::string& filename) {

        OGRRegisterAll();

        OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(driver_name.c_str());
        if (!driver) {
            std::cerr << driver_name << " driver not available.\n";
            exit(1);
        }

        CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "FALSE");
        const char* options[] = { "SPATIALITE=TRUE", nullptr };
        m_data_source = driver->CreateDataSource(filename.c_str(), const_cast<char**>(options));
        if (!m_data_source) {
            std::cerr << "Creation of output file failed.\n";
            exit(1);
        }

        OGRSpatialReference sparef;
        sparef.SetWellKnownGeogCS("WGS84");

        // nodes layer

        m_layer_nodes = m_data_source->CreateLayer("nodes", &sparef, wkbPoint, nullptr);
        if (!m_layer_nodes) {
            std::cerr << "Layer creation failed.\n";
            exit(1);
        }

        OGRFieldDefn layer_nodes_field_id("id", OFTReal);
        layer_nodes_field_id.SetWidth(10);

        if (m_layer_nodes->CreateField(&layer_nodes_field_id) != OGRERR_NONE) {
            std::cerr << "Creating id field failed.\n";
            exit(1);
        }

        // labels layer

        m_layer_labels = m_data_source->CreateLayer("labels", &sparef, wkbPoint, nullptr);
        if (!m_layer_labels) {
            std::cerr << "Layer creation failed.\n";
            exit(1);
        }

        OGRFieldDefn layer_labels_field_id("id", OFTReal);
        layer_labels_field_id.SetWidth(10);

        if (m_layer_labels->CreateField(&layer_labels_field_id) != OGRERR_NONE) {
            std::cerr << "Creating id field failed.\n";
            exit(1);
        }

        OGRFieldDefn layer_labels_field_label("label", OFTString);
        layer_labels_field_label.SetWidth(30);

        if (m_layer_labels->CreateField(&layer_labels_field_label) != OGRERR_NONE) {
            std::cerr << "Creating label field failed.\n";
            exit(1);
        }

        // ways layer

        m_layer_ways = m_data_source->CreateLayer("ways", &sparef, wkbLineString, nullptr);
        if (!m_layer_ways) {
            std::cerr << "Layer creation failed.\n";
            exit(1);
        }

        OGRFieldDefn layer_way_field_id("id", OFTReal);
        layer_way_field_id.SetWidth(10);

        if (m_layer_ways->CreateField(&layer_way_field_id) != OGRERR_NONE) {
            std::cerr << "Creating id field failed.\n";
            exit(1);
        }

        OGRFieldDefn layer_way_field_test("test", OFTInteger);
        layer_way_field_test.SetWidth(3);

        if (m_layer_ways->CreateField(&layer_way_field_test) != OGRERR_NONE) {
            std::cerr << "Creating test field failed.\n";
            exit(1);
        }
    }

    ~TestOverviewHandler() {
        OGRDataSource::DestroyDataSource(m_data_source);
        OGRCleanupAll();
    }

    void node(const osmium::Node& node) {
        const char* label = node.tags().get_value_by_key("label");
        if (label) {
            OGRFeature* feature = OGRFeature::CreateFeature(m_layer_labels->GetLayerDefn());
            std::unique_ptr<OGRPoint> ogr_point = m_factory.create_point(node);
            feature->SetGeometry(ogr_point.get());
            feature->SetField("id", static_cast<double>(node.id()));
            feature->SetField("label", label);

            if (m_layer_labels->CreateFeature(feature) != OGRERR_NONE) {
                std::cerr << "Failed to create feature.\n";
                exit(1);
            }

            OGRFeature::DestroyFeature(feature);
        } else {
            OGRFeature* feature = OGRFeature::CreateFeature(m_layer_nodes->GetLayerDefn());
            std::unique_ptr<OGRPoint> ogr_point = m_factory.create_point(node);
            feature->SetGeometry(ogr_point.get());
            feature->SetField("id", static_cast<double>(node.id()));

            if (m_layer_nodes->CreateFeature(feature) != OGRERR_NONE) {
                std::cerr << "Failed to create feature.\n";
                exit(1);
            }
            OGRFeature::DestroyFeature(feature);
        }
    }

    void way(const osmium::Way& way) {
        try {
            std::unique_ptr<OGRLineString> ogr_linestring = m_factory.create_linestring(way);
            OGRFeature* feature = OGRFeature::CreateFeature(m_layer_ways->GetLayerDefn());
            feature->SetGeometry(ogr_linestring.get());
            feature->SetField("id", static_cast<double>(way.id()));

            const char* test = way.tags().get_value_by_key("test");
            if (test) {
                feature->SetField("test", test);
            }

            if (m_layer_ways->CreateFeature(feature) != OGRERR_NONE) {
                std::cerr << "Failed to create feature.\n";
                exit(1);
            }

            OGRFeature::DestroyFeature(feature);
        } catch (osmium::geometry_error&) {
            std::cerr << "Ignoring illegal geometry for way " << way.id() << ".\n";
        }
    }

};

/* ================================================== */

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " INFILE\n";
        exit(1);
    }

    std::string output_format("SQLite");
    std::string input_filename(argv[1]);
    std::string output_filename("testdata-overview.db");
    ::unlink(output_filename.c_str());

    osmium::io::Reader reader(input_filename);

    index_type index;
    location_handler_type location_handler(index);
    location_handler.ignore_errors();

    TestOverviewHandler handler(output_format, output_filename);

    osmium::apply(reader, location_handler, handler);
    reader.close();
}

