/*

  Convert OSM files from one format into another.

  The code in this example file is released into the Public Domain.

*/

#include <iostream>
#include <getopt.h>

#include <osmium/io/any_input.hpp>

#include <osmium/io/any_output.hpp>

void print_help() {
    std::cout << "osmium_convert [OPTIONS] [INFILE [OUTFILE]]\n\n" \
              << "If INFILE or OUTFILE is not given stdin/stdout is assumed.\n" \
              << "File format is autodetected from file name suffix.\n" \
              << "Use -f and -t options to force file format.\n" \
              << "\nFile types:\n" \
              << "  osm        normal OSM file\n" \
              << "  osc        OSM change file\n" \
              << "  osh        OSM file with history information\n" \
              << "\nFile format:\n" \
              << "  (default)  XML encoding\n" \
              << "  pbf        binary PBF encoding\n" \
              << "  opl        OPL encoding\n" \
              << "\nFile compression\n" \
              << "  gz         compressed with gzip\n" \
              << "  bz2        compressed with bzip2\n" \
              << "\nOptions:\n" \
              << "  -h, --help                This help message\n" \
              << "  -f, --from-format=FORMAT  Input format\n" \
              << "  -t, --to-format=FORMAT    Output format\n";
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"help",        no_argument, 0, 'h'},
        {"from-format", required_argument, 0, 'f'},
        {"to-format",   required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    std::string input_format;
    std::string output_format;

    while (true) {
        int c = getopt_long(argc, argv, "dhf:t:", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                exit(0);
            case 'f':
                input_format = optarg;
                break;
            case 't':
                output_format = optarg;
                break;
            default:
                exit(1);
        }
    }

    std::string input;
    std::string output;
    int remaining_args = argc - optind;
    if (remaining_args > 2) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE [OUTFILE]]" << std::endl;
        exit(1);
    } else if (remaining_args == 2) {
        input =  argv[optind];
        output = argv[optind+1];
    } else if (remaining_args == 1) {
        input =  argv[optind];
    }

    osmium::io::File infile(input, input_format);

    osmium::io::File outfile(output, output_format);

    if (infile.has_multiple_object_versions() && !outfile.has_multiple_object_versions()) {
        std::cerr << "Warning! You are converting from an OSM file with (potentially) several versions of the same object to one that is not marked as such.\n";
    }

    int exit_code = 0;

    try {
        osmium::io::Reader reader(infile);
        osmium::io::Header header = reader.header();
        header.set("generator", "osmium_convert");

        osmium::io::Writer writer(outfile, header, osmium::io::overwrite::allow);
        while (osmium::memory::Buffer buffer = reader.read()) {
            writer(std::move(buffer));
        }
        writer.close();
        reader.close();
    } catch (std::exception& e) {
        std::cerr << e.what() << "\n";
        exit_code = 1;
    }

    google::protobuf::ShutdownProtobufLibrary();
    return exit_code;
}

