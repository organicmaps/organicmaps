#include "catch.hpp"
#include "utils.hpp"

#include <osmium/handler.hpp>
#include <osmium/io/any_compression.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/memory/buffer.hpp>

struct CountHandler : public osmium::handler::Handler {

    int count = 0;

    void node(osmium::Node&) {
        ++count;
    }

}; // class CountHandler

TEST_CASE("Reader") {

    SECTION("reader can be initialized with file") {
        osmium::io::File file(with_data_dir("t/io/data.osm"));
        osmium::io::Reader reader(file);
        osmium::handler::Handler handler;

        osmium::apply(reader, handler);
    }

    SECTION("reader can be initialized with string") {
        osmium::io::Reader reader(with_data_dir("t/io/data.osm"));
        osmium::handler::Handler handler;

        osmium::apply(reader, handler);
    }

    SECTION("should return invalid buffer after eof") {
        osmium::io::File file(with_data_dir("t/io/data.osm"));
        osmium::io::Reader reader(file);

        REQUIRE(!reader.eof());

        while (osmium::memory::Buffer buffer = reader.read()) {
        }

        REQUIRE(reader.eof());

        // extra read always returns invalid buffer
        osmium::memory::Buffer buffer = reader.read();
        REQUIRE(!buffer);
    }

    SECTION("should not hang when apply() is called twice on reader") {
        osmium::io::File file(with_data_dir("t/io/data.osm"));
        osmium::io::Reader reader(file);
        osmium::handler::Handler handler;

        osmium::apply(reader, handler);
        osmium::apply(reader, handler);
    }

    SECTION("should work with a buffer with uncompressed data") {
        int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm"));
        REQUIRE(fd >= 0);

        const size_t buffer_size = 1000;
        char buffer[buffer_size];
        auto length = ::read(fd, buffer, buffer_size);
        REQUIRE(length > 0);

        osmium::io::File file(buffer, static_cast<size_t>(length), "osm");
        osmium::io::Reader reader(file);
        CountHandler handler;

        REQUIRE(handler.count == 0);
        osmium::apply(reader, handler);
        REQUIRE(handler.count == 1);
    }

    SECTION("should work with a buffer with gzip-compressed data") {
        int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm.gz"));
        REQUIRE(fd >= 0);

        const size_t buffer_size = 1000;
        char buffer[buffer_size];
        auto length = ::read(fd, buffer, buffer_size);
        REQUIRE(length > 0);

        osmium::io::File file(buffer, static_cast<size_t>(length), "osm.gz");
        osmium::io::Reader reader(file);
        CountHandler handler;

        REQUIRE(handler.count == 0);
        osmium::apply(reader, handler);
        REQUIRE(handler.count == 1);
    }

    SECTION("should work with a buffer with bzip2-compressed data") {
        int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm.bz2"));
        REQUIRE(fd >= 0);

        const size_t buffer_size = 1000;
        char buffer[buffer_size];
        auto length = ::read(fd, buffer, buffer_size);
        REQUIRE(length > 0);

        osmium::io::File file(buffer, static_cast<size_t>(length), "osm.bz2");
        osmium::io::Reader reader(file);
        CountHandler handler;

        REQUIRE(handler.count == 0);
        osmium::apply(reader, handler);
        REQUIRE(handler.count == 1);
    }

}

