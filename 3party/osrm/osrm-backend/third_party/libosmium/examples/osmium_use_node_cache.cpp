/*

  This reads ways from an OSM file and writes out the node locations
  it got from a node cache generated with osmium_create_node_cache.

  The code in this example file is released into the Public Domain.

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>

#include <osmium/io/pbf_input.hpp>
#include <osmium/io/xml_input.hpp>

#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/dense_mmap_array.hpp>

#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/visitor.hpp>

typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_neg_type;
//typedef osmium::index::map::DenseMmapArray<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;
typedef osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;

typedef osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type> location_handler_type;

class MyHandler : public osmium::handler::Handler {

public:

    void way(osmium::Way& way) {
        for (auto& nr : way.nodes()) {
            std::cout << nr << "\n";
        }
    }

}; // class MyHandler

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " OSM_FILE CACHE_FILE\n";
        return 1;
    }

    std::string input_filename(argv[1]);
    osmium::io::Reader reader(input_filename, osmium::osm_entity_bits::way);

    int fd = open(argv[2], O_RDWR);
    if (fd == -1) {
        std::cerr << "Can not open node cache file '" << argv[2] << "': " << strerror(errno) << "\n";
        return 1;
    }

    index_pos_type index_pos {fd};
    index_neg_type index_neg;
    location_handler_type location_handler(index_pos, index_neg);
    location_handler.ignore_errors();

    MyHandler handler;
    osmium::apply(reader, location_handler, handler);
    reader.close();

    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

