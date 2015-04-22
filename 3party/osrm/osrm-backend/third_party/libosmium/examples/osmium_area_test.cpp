/*

  This is an example tool that creates multipolygons from OSM data
  and dumps them to stdout.

  The code in this example file is released into the Public Domain.

*/

#include <iostream>

#include <getopt.h>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/dynamic_handler.hpp>
#include <osmium/geom/wkt.hpp>
#include <osmium/handler/dump.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>

typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_neg_type;
typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;
typedef osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type> location_handler_type;

class WKTDump : public osmium::handler::Handler {

    osmium::geom::WKTFactory<> m_factory ;

    std::ostream& m_out;

public:

    WKTDump(std::ostream& out) :
        m_out(out) {
    }

    void area(const osmium::Area& area) {
        try {
            m_out << m_factory.create_multipolygon(area) << "\n";
        } catch (osmium::geometry_error& e) {
            m_out << "GEOMETRY ERROR: " << e.what() << "\n";
        }
    }

}; // class WKTDump

void print_help() {
    std::cout << "osmium_area_test [OPTIONS] OSMFILE\n\n"
              << "Read OSMFILE and build multipolygons from it.\n"
              << "\nOptions:\n"
              << "  -h, --help           This help message\n"
              << "  -w, --dump-wkt       Dump area geometries as WKT\n"
              << "  -o, --dump-objects   Dump area objects\n";
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"help",         no_argument, 0, 'h'},
        {"dump-wkt",     no_argument, 0, 'w'},
        {"dump-objects", no_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    osmium::handler::DynamicHandler handler;

    while (true) {
        int c = getopt_long(argc, argv, "hwo", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                exit(0);
            case 'w':
                handler.set<WKTDump>(std::cout);
                break;
            case 'o':
                handler.set<osmium::handler::Dump>(std::cout);
                break;
            default:
                exit(1);
        }
    }

    int remaining_args = argc - optind;
    if (remaining_args != 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE\n";
        exit(1);
    }

    osmium::io::File infile(argv[optind]);

    osmium::area::Assembler::config_type assembler_config;
    osmium::area::MultipolygonCollector<osmium::area::Assembler> collector(assembler_config);

    std::cout << "Pass 1...\n";
    osmium::io::Reader reader1(infile, osmium::osm_entity_bits::relation);
    collector.read_relations(reader1);
    reader1.close();
    std::cout << "Pass 1 done\n";

    std::cout << "Memory:\n";
    collector.used_memory();

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler(index_pos, index_neg);
    location_handler.ignore_errors(); // XXX

    std::cout << "Pass 2...\n";
    osmium::io::Reader reader2(infile);
    osmium::apply(reader2, location_handler, collector.handler([&handler](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, handler);
    }));
    reader2.close();
    std::cout << "Pass 2 done\n";

    std::cout << "Memory:\n";
    collector.used_memory();

    std::vector<const osmium::Relation*> incomplete_relations = collector.get_incomplete_relations();
    if (!incomplete_relations.empty()) {
        std::cerr << "Warning! Some member ways missing for these multipolygon relations:";
        for (const auto* relation : incomplete_relations) {
            std::cerr << " " << relation->id();
        }
        std::cerr << "\n";
    }

    google::protobuf::ShutdownProtobufLibrary();
}

