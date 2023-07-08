#include "catch.hpp"

#include <osmium/util/double.hpp>

TEST_CASE("Double") {

    SECTION("double2string") {
        std::string s1;
        osmium::util::double2string(s1, 1.123, 7);
        REQUIRE(s1 == "1.123");

        std::string s2;
        osmium::util::double2string(s2, 1.000, 7);
        REQUIRE(s2 == "1");

        std::string s3;
        osmium::util::double2string(s3, 0.0, 7);
        REQUIRE(s3 == "0");

        std::string s4;
        osmium::util::double2string(s4, 0.020, 7);
        REQUIRE(s4 == "0.02");

        std::string s5;
        osmium::util::double2string(s5, -0.020, 7);
        REQUIRE(s5 == "-0.02");

        std::string s6;
        osmium::util::double2string(s6, -0.0, 7);
        REQUIRE(s6 == "-0");
    }
}

