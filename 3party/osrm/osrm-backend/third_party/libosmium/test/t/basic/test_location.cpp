#include "catch.hpp"

#include <sstream>
#include <type_traits>

#include <osmium/osm/location.hpp>

TEST_CASE("Location") {

// fails on MSVC and doesn't really matter
// static_assert(std::is_literal_type<osmium::Location>::value, "osmium::Location not literal type");

    SECTION("instantiation_with_default_parameters") {
        osmium::Location loc;
        REQUIRE(!loc);
        REQUIRE_THROWS_AS(loc.lon(), osmium::invalid_location);
        REQUIRE_THROWS_AS(loc.lat(), osmium::invalid_location);
    }

    SECTION("instantiation_with_double_parameters") {
        osmium::Location loc1(1.2, 4.5);
        REQUIRE(!!loc1);
        REQUIRE(12000000 == loc1.x());
        REQUIRE(45000000 == loc1.y());
        REQUIRE(1.2 == loc1.lon());
        REQUIRE(4.5 == loc1.lat());

        osmium::Location loc2(loc1);
        REQUIRE(4.5 == loc2.lat());

        osmium::Location loc3 = loc1;
        REQUIRE(4.5 == loc3.lat());
    }

    SECTION("instantiation_with_double_parameters_constructor_with_universal_initializer") {
        osmium::Location loc { 2.2, 3.3 };
        REQUIRE(2.2 == loc.lon());
        REQUIRE(3.3 == loc.lat());
    }

    SECTION("instantiation_with_double_parameters_constructor_with_initializer_list") {
        osmium::Location loc({ 4.4, 5.5 });
        REQUIRE(4.4 == loc.lon());
        REQUIRE(5.5 == loc.lat());
    }

    SECTION("instantiation_with_double_parameters_operator_equal") {
        osmium::Location loc = { 5.5, 6.6 };
        REQUIRE(5.5 == loc.lon());
        REQUIRE(6.6 == loc.lat());
    }

    SECTION("equality") {
        osmium::Location loc1(1.2, 4.5);
        osmium::Location loc2(1.2, 4.5);
        osmium::Location loc3(1.5, 1.5);
        REQUIRE(loc1 == loc2);
        REQUIRE(loc1 != loc3);
    }

    SECTION("order") {
        REQUIRE(osmium::Location(-1.2, 10.0) < osmium::Location(1.2, 10.0));
        REQUIRE(osmium::Location(1.2, 10.0) > osmium::Location(-1.2, 10.0));

        REQUIRE(osmium::Location(10.2, 20.0) < osmium::Location(11.2, 20.2));
        REQUIRE(osmium::Location(10.2, 20.2) < osmium::Location(11.2, 20.0));
        REQUIRE(osmium::Location(11.2, 20.2) > osmium::Location(10.2, 20.0));
    }

    SECTION("validity") {
        REQUIRE(osmium::Location(0.0, 0.0).valid());
        REQUIRE(osmium::Location(1.2, 4.5).valid());
        REQUIRE(osmium::Location(-1.2, 4.5).valid());
        REQUIRE(osmium::Location(-180.0, -90.0).valid());
        REQUIRE(osmium::Location(180.0, -90.0).valid());
        REQUIRE(osmium::Location(-180.0, 90.0).valid());
        REQUIRE(osmium::Location(180.0, 90.0).valid());

        REQUIRE(!osmium::Location(200.0, 4.5).valid());
        REQUIRE(!osmium::Location(-1.2, -100.0).valid());
        REQUIRE(!osmium::Location(-180.0, 90.005).valid());
    }


    SECTION("output_to_iterator_comma_separator") {
        char buffer[100];
        osmium::Location loc(-3.2, 47.3);
        *loc.as_string(buffer, ',') = 0;
        REQUIRE(std::string("-3.2,47.3") == buffer);
    }

    SECTION("output_to_iterator_space_separator") {
        char buffer[100];
        osmium::Location loc(0.0, 7.0);
        *loc.as_string(buffer, ' ') = 0;
        REQUIRE(std::string("0 7") == buffer);
    }

    SECTION("output_to_iterator_check_precision") {
        char buffer[100];
        osmium::Location loc(-179.9999999, -90.0);
        *loc.as_string(buffer, ' ') = 0;
        REQUIRE(std::string("-179.9999999 -90") == buffer);
    }

    SECTION("output_to_iterator_undefined_location") {
        char buffer[100];
        osmium::Location loc;
        REQUIRE_THROWS_AS(loc.as_string(buffer, ','), osmium::invalid_location);
    }

    SECTION("output_to_string_comman_separator") {
        std::string s;
        osmium::Location loc(-3.2, 47.3);
        loc.as_string(std::back_inserter(s), ',');
        REQUIRE(s == "-3.2,47.3");
    }

    SECTION("output_to_string_space_separator") {
        std::string s;
        osmium::Location loc(0.0, 7.0);
        loc.as_string(std::back_inserter(s), ' ');
        REQUIRE(s == "0 7");
    }

    SECTION("output_to_string_check_precision") {
        std::string s;
        osmium::Location loc(-179.9999999, -90.0);
        loc.as_string(std::back_inserter(s), ' ');
        REQUIRE(s == "-179.9999999 -90");
    }

    SECTION("output_to_string_undefined_location") {
        std::string s;
        osmium::Location loc;
        REQUIRE_THROWS_AS(loc.as_string(std::back_inserter(s), ','), osmium::invalid_location);
    }

    SECTION("output_defined") {
        osmium::Location p(-3.2, 47.3);
        std::stringstream out;
        out << p;
        REQUIRE(out.str() == "(-3.2,47.3)");
    }

    SECTION("output_undefined") {
        osmium::Location p;
        std::stringstream out;
        out << p;
        REQUIRE(out.str() == "(undefined,undefined)");
    }

}

