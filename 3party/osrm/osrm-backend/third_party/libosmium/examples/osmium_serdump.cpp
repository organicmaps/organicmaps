/*

  This is a small tool to dump the contents of the input file
  in serialized format to stdout.

  The code in this example file is released into the Public Domain.

*/

#include <cerrno>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _MSC_VER
# include <direct.h>
#endif

#include <osmium/io/pbf_input.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/handler/disk_store.hpp>
#include <osmium/handler/object_relations.hpp>

#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/multimap/sparse_mem_multimap.hpp>
#include <osmium/index/multimap/sparse_mem_array.hpp>
#include <osmium/index/multimap/hybrid.hpp>

// ==============================================================================
// Choose the following depending on the size of the input OSM files:
// ==============================================================================
// for smaller OSM files (extracts)
typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, size_t> offset_index_type;
//typedef osmium::index::map::SparseMapMmap<osmium::unsigned_object_id_type, size_t> offset_index_type;
//typedef osmium::index::map::SparseMapFile<osmium::unsigned_object_id_type, size_t> offset_index_type;

typedef osmium::index::multimap::SparseMemArray<osmium::unsigned_object_id_type, osmium::unsigned_object_id_type> map_type;
//typedef osmium::index::multimap::SparseMemMultimap<osmium::unsigned_object_id_type, osmium::unsigned_object_id_type> map_type;
//typedef osmium::index::multimap::Hybrid<osmium::unsigned_object_id_type, osmium::unsigned_object_id_type> map_type;

// ==============================================================================
// for very large OSM files (planet)
//typedef osmium::index::map::DenseMmapArray<osmium::unsigned_object_id_type, size_t> offset_index_type;
// ==============================================================================

void print_help() {
    std::cout << "osmium_serdump OSMFILE DIR\n" \
              << "Serialize content of OSMFILE into data file in DIR.\n" \
              << "\nOptions:\n" \
              << "  -h, --help       This help message\n";
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    static struct option long_options[] = {
        {"help",      no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while (true) {
        int c = getopt_long(argc, argv, "h", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                exit(0);
            default:
                exit(2);
        }
    }

    int remaining_args = argc - optind;

    if (remaining_args != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE DIR\n";
        exit(2);
    }

    std::string dir(argv[optind+1]);
#ifndef _WIN32
    int result = ::mkdir(dir.c_str(), 0777);
#else
    int result = mkdir(dir.c_str());
#endif
    if (result == -1 && errno != EEXIST) {
        std::cerr << "Problem creating directory '" << dir << "': " << strerror(errno) << "\n";
        exit(2);
    }

    std::string data_file(dir + "/data.osm.ser");
    int data_fd = ::open(data_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (data_fd < 0) {
        std::cerr << "Can't open data file '" << data_file << "': " << strerror(errno) << "\n";
        exit(2);
    }

    offset_index_type node_index;
    offset_index_type way_index;
    offset_index_type relation_index;

    osmium::handler::DiskStore disk_store_handler(data_fd, node_index, way_index, relation_index);

    map_type map_node2way;
    map_type map_node2relation;
    map_type map_way2relation;
    map_type map_relation2relation;

    osmium::handler::ObjectRelations object_relations_handler(map_node2way, map_node2relation, map_way2relation, map_relation2relation);

    osmium::io::Reader reader(argv[1]);

    while (osmium::memory::Buffer buffer = reader.read()) {
        disk_store_handler(buffer); // XXX
        osmium::apply(buffer, object_relations_handler);
    }

    reader.close();

    {
        std::string index_file(dir + "/nodes.idx");
        int fd = ::open(index_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            std::cerr << "Can't open nodes index file '" << index_file << "': " << strerror(errno) << "\n";
            exit(2);
        }
        node_index.dump_as_list(fd);
        close(fd);
    }

    {
        std::string index_file(dir + "/ways.idx");
        int fd = ::open(index_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            std::cerr << "Can't open ways index file '" << index_file << "': " << strerror(errno) << "\n";
            exit(2);
        }
        way_index.dump_as_list(fd);
        close(fd);
    }

    {
        std::string index_file(dir + "/relations.idx");
        int fd = ::open(index_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            std::cerr << "Can't open relations index file '" << index_file << "': " << strerror(errno) << "\n";
            exit(2);
        }
        relation_index.dump_as_list(fd);
        close(fd);
    }

    {
        map_node2way.sort();
        std::string index_file(dir + "/node2way.map");
        int fd = ::open(index_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            std::cerr << "Can't open node->way map file '" << index_file << "': " << strerror(errno) << "\n";
            exit(2);
        }
        map_node2way.dump_as_list(fd);
        close(fd);
    }

    {
        map_node2relation.sort();
        std::string index_file(dir + "/node2rel.map");
        int fd = ::open(index_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            std::cerr << "Can't open node->rel map file '" << index_file << "': " << strerror(errno) << "\n";
            exit(2);
        }
        map_node2relation.dump_as_list(fd);
        close(fd);
    }

    {
        map_way2relation.sort();
        std::string index_file(dir + "/way2rel.map");
        int fd = ::open(index_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            std::cerr << "Can't open way->rel map file '" << index_file << "': " << strerror(errno) << "\n";
            exit(2);
        }
        map_way2relation.dump_as_list(fd);
        close(fd);
    }

    {
        map_relation2relation.sort();
        std::string index_file(dir + "/rel2rel.map");
        int fd = ::open(index_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            std::cerr << "Can't open rel->rel map file '" << index_file << "': " << strerror(errno) << "\n";
            exit(2);
        }
        map_relation2relation.dump_as_list(fd);
        close(fd);
    }

    google::protobuf::ShutdownProtobufLibrary();
}

