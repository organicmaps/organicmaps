#include "catch.hpp"

#include <sstream>

#include <osmium/osm/timestamp.hpp>

TEST_CASE("Timestamp") {

    SECTION("can be default initialized to invalid value") {
        osmium::Timestamp t;
        REQUIRE(0 == t);
        REQUIRE("" == t.to_iso());
    }

    SECTION("invalid value is zero") {
        osmium::Timestamp t(static_cast<time_t>(0));
        REQUIRE(0 == t);
        REQUIRE("" == t.to_iso());
    }

    SECTION("can be initialized from time_t") {
        osmium::Timestamp t(static_cast<time_t>(1));
        REQUIRE(1 == t);
        REQUIRE("1970-01-01T00:00:01Z" == t.to_iso());
    }

    SECTION("can be initialized from string") {
        osmium::Timestamp t("2000-01-01T00:00:00Z");
        REQUIRE("2000-01-01T00:00:00Z" == t.to_iso());
    }

    SECTION("can be implicitly cast to time_t") {
        osmium::Timestamp t(4242);
        time_t x = t;
        REQUIRE(x == 4242);
    }

    SECTION("uint32_t can be initialized from Timestamp") {
        osmium::Timestamp t(4242);
        uint32_t x { t };

        REQUIRE(x == 4242);
    }

    SECTION("can be compared") {
        osmium::Timestamp t1(10);
        osmium::Timestamp t2(50);
        REQUIRE(t1 < t2);
    }

    SECTION("can be written to stream") {
        std::stringstream ss;
        osmium::Timestamp t(1);
        ss << t;
        REQUIRE("1970-01-01T00:00:01Z" == ss.str());
    }

}
