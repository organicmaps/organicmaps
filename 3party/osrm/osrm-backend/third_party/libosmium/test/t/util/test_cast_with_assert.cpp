#include "catch.hpp"

// Define assert() to throw this error. This enables the tests to check that
// the assert() fails.
struct assert_error : public std::runtime_error {
    assert_error(const char* what_arg) : std::runtime_error(what_arg) {
    }
};
#define assert(x) if (!(x)) { throw(assert_error(#x)); }

#include <osmium/util/cast.hpp>

TEST_CASE("static_cast_with_assert") {

    SECTION("same types is always okay") {
        int f = 10;
        auto t = osmium::static_cast_with_assert<int>(f);
        REQUIRE(t == f);
    }

    SECTION("casting to larger type is always okay") {
        int16_t f = 10;
        auto t = osmium::static_cast_with_assert<int32_t>(f);
        REQUIRE(t == f);
    }


    SECTION("cast int32_t -> int_16t should not trigger assert for small int") {
        int32_t f = 100;
        auto t = osmium::static_cast_with_assert<int16_t>(f);
        REQUIRE(t == f);
    }

    SECTION("cast int32_t -> int_16t should trigger assert for large int") {
        int32_t f = 100000;
        REQUIRE_THROWS_AS(osmium::static_cast_with_assert<int16_t>(f), assert_error);
    }


    SECTION("cast int16_t -> uint16_t should not trigger assert for zero") {
        int16_t f = 0;
        auto t = osmium::static_cast_with_assert<uint16_t>(f);
        REQUIRE(t == f);
    }

    SECTION("cast int16_t -> uint16_t should not trigger assert for positive int") {
        int16_t f = 1;
        auto t = osmium::static_cast_with_assert<uint16_t>(f);
        REQUIRE(t == f);
    }

    SECTION("cast int16_t -> uint16_t should trigger assert for negative int") {
        int16_t f = -1;
        REQUIRE_THROWS_AS(osmium::static_cast_with_assert<uint16_t>(f), assert_error);
    }


    SECTION("cast uint32_t -> uint_16t should not trigger assert for zero") {
        uint32_t f = 0;
        auto t = osmium::static_cast_with_assert<uint16_t>(f);
        REQUIRE(t == f);
    }

    SECTION("cast uint32_t -> uint_16t should not trigger assert for small int") {
        uint32_t f = 100;
        auto t = osmium::static_cast_with_assert<uint16_t>(f);
        REQUIRE(t == f);
    }

    SECTION("cast int32_t -> int_16t should trigger assert for large int") {
        uint32_t f = 100000;
        REQUIRE_THROWS_AS(osmium::static_cast_with_assert<uint16_t>(f), assert_error);
    }


    SECTION("cast uint16_t -> int16_t should not trigger assert for small int") {
        uint16_t f = 1;
        auto t = osmium::static_cast_with_assert<int16_t>(f);
        REQUIRE(t == f);
    }

    SECTION("cast uint16_t -> int16_t should trigger assert for large int") {
        uint16_t f = 65000;
        REQUIRE_THROWS_AS(osmium::static_cast_with_assert<int16_t>(f), assert_error);
    }


}

