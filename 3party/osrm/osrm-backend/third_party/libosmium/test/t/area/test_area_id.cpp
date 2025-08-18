#include "catch.hpp"

#include <osmium/osm/area.hpp>

TEST_CASE("area_id") {

    SECTION("object_id_to_area_id_conversion") {
        REQUIRE( 46 == osmium::object_id_to_area_id( 23, osmium::item_type::way));
        REQUIRE( 47 == osmium::object_id_to_area_id( 23, osmium::item_type::relation));
        REQUIRE(  0 == osmium::object_id_to_area_id(  0, osmium::item_type::way));
        REQUIRE(  1 == osmium::object_id_to_area_id(  0, osmium::item_type::relation));
        REQUIRE(-24 == osmium::object_id_to_area_id(-12, osmium::item_type::way));
        REQUIRE(-25 == osmium::object_id_to_area_id(-12, osmium::item_type::relation));
    }

    SECTION("area_id_to_object_id_conversion") {
        REQUIRE( 23 == osmium::area_id_to_object_id( 46));
        REQUIRE( 23 == osmium::area_id_to_object_id( 47));
        REQUIRE(  0 == osmium::area_id_to_object_id(  0));
        REQUIRE(  0 == osmium::area_id_to_object_id(  1));
        REQUIRE(-12 == osmium::area_id_to_object_id(-24));
        REQUIRE(-12 == osmium::area_id_to_object_id(-25));
    }

}
