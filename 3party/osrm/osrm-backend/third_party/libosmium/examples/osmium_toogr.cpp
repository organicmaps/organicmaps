/*

  This is an example tool that converts OSM data to some output format
  like Spatialite or Shapefiles using the OGR library.

  The code in this example file is released into the Public Domain.

*/

#include <iostream>
#include <getopt.h>

#include <osmium/index/map/all.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/visitor.hpp>

#include <osmium/geom/ogr.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>

typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_neg_type;
typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;

typedef osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type> location_handler_type;

class MyOGRHandler : public osmium::handler::Handler {

    OGRDataSource* m_data_source;
    OGRLayer* m_layer_point;
    OGRLayer* m_layer_linestring;

    osmium::geom::OGRFactory<> m_factory;

public:

    MyOGRHandler(const std::string& driver_name, const std::string& filename) {

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
        m_layer_point = m_data_source->CreateLayer("postboxes", &sparef, wkbPoint, nullptr);
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

        OGRFieldDefn layer_point_field_operator("operator", OFTString);
        layer_point_field_operator.SetWidth(30);

        if (m_layer_point->CreateField(&layer_point_field_operator) != OGRERR_NONE) {
            std::cerr << "Creating operator field failed.\n";
            exit(1);
        }

        /* Transactions might make things faster, then again they might not.
           Feel free to experiment and benchmark and report back. */
        m_layer_point->StartTransaction();

        m_layer_linestring = m_data_source->CreateLayer("roads", &sparef, wkbLineString, nullptr);
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

        m_layer_linestring->StartTransaction();
    }

    ~MyOGRHandler() {
        m_layer_linestring->CommitTransaction();
        m_layer_point->CommitTransaction();
        OGRDataSource::DestroyDataSource(m_data_source);
        OGRCleanupAll();
    }

    void node(const osmium::Node& node) {
        const char* amenity = node.tags().get_value_by_key("amenity");
        if (amenity && !strcmp(amenity, "post_box")) {
            OGRFeature* feature = OGRFeature::CreateFeature(m_layer_point->GetLayerDefn());
            std::unique_ptr<OGRPoint> ogr_point = m_factory.create_point(node);
            feature->SetGeometry(ogr_point.get());
            feature->SetField("id", static_cast<double>(node.id()));
            feature->SetField("operator", node.tags().get_value_by_key("operator"));

            if (m_layer_point->CreateFeature(feature) != OGRERR_NONE) {
                std::cerr << "Failed to create feature.\n";
                exit(1);
            }

            OGRFeature::DestroyFeature(feature);
        }
    }

    void way(const osmium::Way& way) {
        const char* highway = way.tags().get_value_by_key("highway");
        if (highway) {
            try {
                std::unique_ptr<OGRLineString> ogr_linestring = m_factory.create_linestring(way);
                OGRFeature* feature = OGRFeature::CreateFeature(m_layer_linestring->GetLayerDefn());
                feature->SetGeometry(ogr_linestring.get());
                feature->SetField("id", static_cast<double>(way.id()));
                feature->SetField("type", highway);

                if (m_layer_linestring->CreateFeature(feature) != OGRERR_NONE) {
                    std::cerr << "Failed to create feature.\n";
                    exit(1);
                }

                OGRFeature::DestroyFeature(feature);
            } catch (osmium::geometry_error&) {
                std::cerr << "Ignoring illegal geometry for way " << way.id() << ".\n";
            }
        }
    }

};

/* ================================================== */

void print_help() {
    std::cout << "osmium_toogr [OPTIONS] [INFILE [OUTFILE]]\n\n" \
              << "If INFILE is not given stdin is assumed.\n" \
              << "If OUTFILE is not given 'ogr_out' is used.\n" \
              << "\nOptions:\n" \
              << "  -h, --help                 This help message\n" \
              << "  -l, --location_store=TYPE  Set location store\n" \
              << "  -f, --format=FORMAT        Output OGR format (Default: 'SQLite')\n" \
              << "  -L                         See available location stores\n";
}

int main(int argc, char* argv[]) {
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    static struct option long_options[] = {
        {"help",                 no_argument, 0, 'h'},
        {"format",               required_argument, 0, 'f'},
        {"location_store",       required_argument, 0, 'l'},
        {"list_location_stores", no_argument, 0, 'L'},
        {0, 0, 0, 0}
    };

    std::string output_format { "SQLite" };
    std::string location_store { "sparse_mem_array" };

    while (true) {
        int c = getopt_long(argc, argv, "hf:l:L", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                exit(0);
            case 'f':
                output_format = optarg;
                break;
            case 'l':
                location_store = optarg;
                break;
            case 'L':
                std::cout << "Available map types:\n";
                for (const auto& map_type : map_factory.map_types()) {
                    std::cout << "  " << map_type << "\n";
                }
                exit(0);
            default:
                exit(1);
        }
    }

    std::string input_filename;
    std::string output_filename("ogr_out");
    int remaining_args = argc - optind;
    if (remaining_args > 2) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE [OUTFILE]]" << std::endl;
        exit(1);
    } else if (remaining_args == 2) {
        input_filename =  argv[optind];
        output_filename = argv[optind+1];
    } else if (remaining_args == 1) {
        input_filename =  argv[optind];
    } else {
        input_filename = "-";
    }

    osmium::io::Reader reader(input_filename);

    std::unique_ptr<index_pos_type> index_pos = map_factory.create_map(location_store);
    index_neg_type index_neg;
    location_handler_type location_handler(*index_pos, index_neg);
    location_handler.ignore_errors();

    MyOGRHandler ogr_handler(output_format, output_filename);

    osmium::apply(reader, location_handler, ogr_handler);
    reader.close();

    google::protobuf::ShutdownProtobufLibrary();

    int locations_fd = open("locations.dump", O_WRONLY | O_CREAT, 0644);
    if (locations_fd < 0) {
        throw std::system_error(errno, std::system_category(), "Open failed");
    }
    index_pos->dump_as_list(locations_fd);
    close(locations_fd);
}

