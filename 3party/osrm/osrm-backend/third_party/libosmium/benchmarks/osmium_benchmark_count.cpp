/*

  The code in this file is released into the Public Domain.

*/

#include <iostream>

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

struct CountHandler : public osmium::handler::Handler {

    int nodes = 0;
    int ways = 0;
    int relations = 0;

    void node(osmium::Node&) {
        ++nodes;
    }

    void way(osmium::Way&) {
        ++ways;
    }

    void relation(osmium::Relation&) {
        ++relations;
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

    std::cout << "Nodes: "     << handler.nodes << "\n";
    std::cout << "Ways: "      << handler.ways << "\n";
    std::cout << "Relations: " << handler.relations << "\n";

    google::protobuf::ShutdownProtobufLibrary();
}

