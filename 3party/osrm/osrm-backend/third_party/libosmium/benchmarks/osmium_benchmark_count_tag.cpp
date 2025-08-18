/*

  The code in this file is released into the Public Domain.

*/

#include <iostream>

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

struct CountHandler : public osmium::handler::Handler {

    int counter = 0;
    int all = 0;

    void node(osmium::Node& node) {
        ++all;
        const char* amenity = node.tags().get_value_by_key("amenity");
        if (amenity && !strcmp(amenity, "post_box")) {
            ++counter;
        }
    }

    void way(osmium::Way&) {
        ++all;
    }

    void relation(osmium::Relation&) {
        ++all;
    }

};


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        exit(1);
    }

    std::string input_filename = argv[1];

    osmium::io::Reader reader(input_filename);

    CountHandler handler;
    osmium::apply(reader, handler);
    reader.close();

    std::cout << "r_all=" << handler.all << " r_counter="  << handler.counter << "\n";

    google::protobuf::ShutdownProtobufLibrary();
}

