/*

  This benchmarks compares the run time for statically vs. dynamically
  configured index maps. You can configure index maps at compile-time using
  typedefs or at run-time using polymorphism.

  This will read the input file into a buffer and then run the
  NodeLocationForWays handler multiple times over the complete data. The
  number of runs depends on the size of the input, but is never smaller
  than 10.

  Do not run this with very large input files! It will need about 10 times
  as much RAM as the file size of the input file.

  The code in this file is released into the Public Domain.

*/

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>

#include <osmium/index/map/all.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/visitor.hpp>

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>

typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location> static_index_type;
const std::string location_store="sparse_mem_array";

typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> dynamic_index_type;

typedef osmium::handler::NodeLocationsForWays<static_index_type> static_location_handler_type;
typedef osmium::handler::NodeLocationsForWays<dynamic_index_type> dynamic_location_handler_type;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        exit(1);
    }

    std::string input_filename = argv[1];

    osmium::memory::Buffer buffer = osmium::io::read_file(input_filename);
    google::protobuf::ShutdownProtobufLibrary();

    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    const auto buffer_size = buffer.committed() / (1024*1024); // buffer size in MBytes
    const int runs = std::max(10, static_cast<int>(5000ull / buffer_size));

    std::cout << "input: filename=" << input_filename << " buffer_size=" << buffer_size << "MBytes\n";
    std::cout << "runs: " << runs << "\n";

    double static_min = std::numeric_limits<double>::max();
    double static_sum = 0;
    double static_max = 0;

    double dynamic_min = std::numeric_limits<double>::max();
    double dynamic_sum = 0;
    double dynamic_max = 0;

    for (int i = 0; i < runs; ++i) {

        {
            // static index
            osmium::memory::Buffer tmp_buffer(buffer.committed());
            for (const auto& item : buffer) {
                tmp_buffer.add_item(item);
                tmp_buffer.commit();
            }

            static_index_type static_index;
            static_location_handler_type static_location_handler(static_index);

            auto start = std::chrono::steady_clock::now();
            osmium::apply(tmp_buffer, static_location_handler);
            auto end = std::chrono::steady_clock::now();

            double duration = std::chrono::duration<double, std::milli>(end-start).count();

            if (duration < static_min) static_min = duration;
            if (duration > static_max) static_max = duration;
            static_sum += duration;
        }

        {
            // dynamic index
            osmium::memory::Buffer tmp_buffer(buffer.committed());
            for (const auto& item : buffer) {
                tmp_buffer.add_item(item);
                tmp_buffer.commit();
            }

            std::unique_ptr<dynamic_index_type> index = map_factory.create_map(location_store);
            dynamic_location_handler_type dynamic_location_handler(*index);
            dynamic_location_handler.ignore_errors();

            auto start = std::chrono::steady_clock::now();
            osmium::apply(tmp_buffer, dynamic_location_handler);
            auto end = std::chrono::steady_clock::now();

            double duration = std::chrono::duration<double, std::milli>(end-start).count();

            if (duration < dynamic_min) dynamic_min = duration;
            if (duration > dynamic_max) dynamic_max = duration;
            dynamic_sum += duration;
        }
    }

    double static_avg = static_sum/runs;
    double dynamic_avg = dynamic_sum/runs;

    std::cout << "static  min=" << static_min << "ms avg=" << static_avg << "ms max=" << static_max << "ms\n";
    std::cout << "dynamic min=" << dynamic_min << "ms avg=" << dynamic_avg << "ms max=" << dynamic_max << "ms\n";

    double rfactor = 100.0;
    double diff_min = std::round((dynamic_min - static_min) * rfactor) / rfactor;
    double diff_avg = std::round((dynamic_avg - static_avg) * rfactor) / rfactor;
    double diff_max = std::round((dynamic_max - static_max) * rfactor) / rfactor;

    double prfactor = 10.0;
    double percent_min = std::round((100.0 * diff_min / static_min) * prfactor) / prfactor;
    double percent_avg = std::round((100.0 * diff_avg / static_avg) * prfactor) / prfactor;
    double percent_max = std::round((100.0 * diff_max / static_max) * prfactor) / prfactor;

    std::cout << "difference:";
    std::cout << " min=" << diff_min << "ms (" << percent_min << "%)";
    std::cout << " avg=" << diff_avg << "ms (" << percent_avg << "%)";
    std::cout << " max=" << diff_max << "ms (" << percent_max << "%)\n";
}

