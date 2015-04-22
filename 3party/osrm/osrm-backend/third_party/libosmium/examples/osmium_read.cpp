/*

  This is a small tool that reads and discards the contents of the input file.
  (Used for timing.)

  The code in this example file is released into the Public Domain.

*/

#include <iostream>

#include <osmium/io/any_input.hpp>

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        exit(1);
    }

    osmium::io::File infile(argv[1]);
    osmium::io::Reader reader(infile);

    while (osmium::memory::Buffer buffer = reader.read()) {
        // do nothing
    }

    reader.close();

    google::protobuf::ShutdownProtobufLibrary();
}

