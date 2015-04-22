#include "catch.hpp"

#include <iterator>

#include <osmium/io/file.hpp>

TEST_CASE("FileFormats") {

    SECTION("default_file_format") {
        osmium::io::File f;
        REQUIRE(osmium::io::file_format::unknown == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        REQUIRE_THROWS_AS(f.check(), std::runtime_error);
    }

    SECTION("stdin_stdout_empty") {
        osmium::io::File f {""};
        REQUIRE(osmium::io::file_format::unknown == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        REQUIRE_THROWS_AS(f.check(), std::runtime_error);
    }

    SECTION("stdin_stdout_dash") {
        osmium::io::File f {"-"};
        REQUIRE(osmium::io::file_format::unknown == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        REQUIRE_THROWS_AS(f.check(), std::runtime_error);
    }

    SECTION("stdin_stdout_bz2") {
        osmium::io::File f {"-", "osm.bz2"};
        REQUIRE("" == f.filename());
        REQUIRE(osmium::io::file_format::xml == f.format());
        REQUIRE(osmium::io::file_compression::bzip2 == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("detect_file_format_by_suffix_osm") {
        osmium::io::File f {"test.osm"};
        REQUIRE(osmium::io::file_format::xml == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("detect_file_format_by_suffix_pbf") {
        osmium::io::File f {"test.pbf"};
        REQUIRE(osmium::io::file_format::pbf == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("detect_file_format_by_suffix_osm_pbf") {
        osmium::io::File f {"test.osm.pbf"};
        REQUIRE(osmium::io::file_format::pbf == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("detect_file_format_by_suffix_opl") {
        osmium::io::File f {"test.opl"};
        REQUIRE(osmium::io::file_format::opl == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("detect_file_format_by_suffix_osm_opl") {
        osmium::io::File f {"test.osm.opl"};
        REQUIRE(osmium::io::file_format::opl == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("detect_file_format_by_suffix_osm_gz") {
        osmium::io::File f {"test.osm.gz"};
        REQUIRE(osmium::io::file_format::xml == f.format());
        REQUIRE(osmium::io::file_compression::gzip == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("detect_file_format_by_suffix_opl_bz2") {
        osmium::io::File f {"test.osm.opl.bz2"};
        REQUIRE(osmium::io::file_format::opl == f.format());
        REQUIRE(osmium::io::file_compression::bzip2 == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("detect_file_format_by_suffix_osc_gz") {
        osmium::io::File f {"test.osc.gz"};
        REQUIRE(osmium::io::file_format::xml == f.format());
        REQUIRE(osmium::io::file_compression::gzip == f.compression());
        REQUIRE(true == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("detect_file_format_by_suffix_opl_gz") {
        osmium::io::File f {"test.osh.opl.gz"};
        REQUIRE(osmium::io::file_format::opl == f.format());
        REQUIRE(osmium::io::file_compression::gzip == f.compression());
        REQUIRE(true == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("detect_file_format_by_suffix_osh_pbf") {
        osmium::io::File f {"test.osh.pbf"};
        REQUIRE(osmium::io::file_format::pbf == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(true == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("override_file_format_by_suffix_osm") {
        osmium::io::File f {"test", "osm"};
        REQUIRE(osmium::io::file_format::xml == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("override_file_format_by_suffix_pbf") {
        osmium::io::File f {"test", "pbf"};
        REQUIRE(osmium::io::file_format::pbf == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("override_file_format_by_suffix_osm_pbf") {
        osmium::io::File f {"test", "osm.pbf"};
        REQUIRE(osmium::io::file_format::pbf == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("override_file_format_by_suffix_opl") {
        osmium::io::File f {"test", "opl"};
        REQUIRE(osmium::io::file_format::opl == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("override_file_format_by_suffix_osm_opl") {
        osmium::io::File f {"test", "osm.opl"};
        REQUIRE(osmium::io::file_format::opl == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("override_file_format_by_suffix_osm_gz") {
        osmium::io::File f {"test", "osm.gz"};
        REQUIRE(osmium::io::file_format::xml == f.format());
        REQUIRE(osmium::io::file_compression::gzip == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("override_file_format_by_suffix_osm_opl_bz2") {
        osmium::io::File f {"test", "osm.opl.bz2"};
        REQUIRE(osmium::io::file_format::opl == f.format());
        REQUIRE(osmium::io::file_compression::bzip2 == f.compression());
        REQUIRE(false == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("override_file_format_by_suffix_osc_gz") {
        osmium::io::File f {"test", "osc.gz"};
        REQUIRE(osmium::io::file_format::xml == f.format());
        REQUIRE(osmium::io::file_compression::gzip == f.compression());
        REQUIRE(true == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("override_file_format_by_suffix_osh_opl_gz") {
        osmium::io::File f {"test", "osh.opl.gz"};
        REQUIRE(osmium::io::file_format::opl == f.format());
        REQUIRE(osmium::io::file_compression::gzip == f.compression());
        REQUIRE(true == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("override_file_format_by_suffix_osh_pbf") {
        osmium::io::File f {"test", "osh.pbf"};
        REQUIRE(osmium::io::file_format::pbf == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(true == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("format_options_pbf_history") {
        osmium::io::File f {"test", "pbf,history=true"};
        REQUIRE(osmium::io::file_format::pbf == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE(true == f.has_multiple_object_versions());
        f.check();
    }

    SECTION("format_options_pbf_foo") {
        osmium::io::File f {"test.osm", "pbf,foo=bar"};
        REQUIRE(osmium::io::file_format::pbf == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE("bar" == f.get("foo"));
        f.check();
    }

    SECTION("format_options_xml_abc_something") {
        osmium::io::File f {"test.bla", "xml,abc,some=thing"};
        REQUIRE(osmium::io::file_format::xml == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE("true" == f.get("abc"));
        REQUIRE("thing" == f.get("some"));
        REQUIRE(2 == std::distance(f.begin(), f.end()));
        f.check();
    }

    SECTION("unknown_format_foo_bar") {
        osmium::io::File f {"test.foo.bar"};
        REQUIRE(osmium::io::file_format::unknown == f.format());
        REQUIRE(osmium::io::file_compression::none == f.compression());
        REQUIRE_THROWS_AS(f.check(), std::runtime_error);
    }

    SECTION("unknown_format_foo") {
        osmium::io::File f {"test", "foo"};
        REQUIRE_THROWS_AS(f.check(), std::runtime_error);
    }

    SECTION("unknown_format_osm_foo") {
        osmium::io::File f {"test", "osm.foo"};
        REQUIRE_THROWS_AS(f.check(), std::runtime_error);
    }

    SECTION("unknown_format_bla_equals_foo") {
        osmium::io::File f {"test", "bla=foo"};
        REQUIRE_THROWS_AS(f.check(), std::runtime_error);
    }

}

