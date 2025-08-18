#include "catch.hpp"

#include <osmium/builder/builder_helper.hpp>
#include <osmium/geom/wkt.hpp>

#include "../basic/helper.hpp"

TEST_CASE("WKT_Geometry") {

SECTION("point") {
    osmium::geom::WKTFactory<> factory;

    std::string wkt {factory.create_point(osmium::Location(3.2, 4.2))};
    REQUIRE(std::string{"POINT(3.2 4.2)"} == wkt);
}

SECTION("empty_point") {
    osmium::geom::WKTFactory<> factory;

    REQUIRE_THROWS_AS(factory.create_point(osmium::Location()), osmium::invalid_location);
}

SECTION("linestring") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    auto& wnl = osmium::builder::build_way_node_list(buffer, {
        {1, {3.2, 4.2}},
        {3, {3.5, 4.7}},
        {4, {3.5, 4.7}},
        {2, {3.6, 4.9}}
    });

    {
        std::string wkt {factory.create_linestring(wnl)};
        REQUIRE(std::string{"LINESTRING(3.2 4.2,3.5 4.7,3.6 4.9)"} == wkt);
    }

    {
        std::string wkt {factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward)};
        REQUIRE(std::string{"LINESTRING(3.6 4.9,3.5 4.7,3.2 4.2)"} == wkt);
    }

    {
        std::string wkt {factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
        REQUIRE(std::string{"LINESTRING(3.2 4.2,3.5 4.7,3.5 4.7,3.6 4.9)"} == wkt);
    }

    {
        std::string wkt {factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
        REQUIRE(std::string{"LINESTRING(3.6 4.9,3.5 4.7,3.5 4.7,3.2 4.2)"} == wkt);
    }
}

SECTION("empty_linestring") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    auto& wnl = osmium::builder::build_way_node_list(buffer, {});

    REQUIRE_THROWS_AS(factory.create_linestring(wnl), osmium::geometry_error);
    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward), osmium::geometry_error);
    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::all), osmium::geometry_error);
    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward), osmium::geometry_error);
}

SECTION("linestring_with_two_same_locations") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    auto& wnl = osmium::builder::build_way_node_list(buffer, {
        {1, {3.5, 4.7}},
        {2, {3.5, 4.7}}
    });

    REQUIRE_THROWS_AS(factory.create_linestring(wnl), osmium::geometry_error);
    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward), osmium::geometry_error);

    {
        std::string wkt {factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
        REQUIRE(std::string{"LINESTRING(3.5 4.7,3.5 4.7)"} == wkt);
    }

    {
        std::string wkt {factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
        REQUIRE(std::string{"LINESTRING(3.5 4.7,3.5 4.7)"} == wkt);
    }
}

SECTION("linestring_with_undefined_location") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    auto& wnl = osmium::builder::build_way_node_list(buffer, {
        {1, {3.5, 4.7}},
        {2, osmium::Location()}
    });

    REQUIRE_THROWS_AS(factory.create_linestring(wnl), osmium::invalid_location);
}

SECTION("area_1outer_0inner") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    osmium::Area& area = buffer_add_area(buffer,
        "foo",
        {},
        {
            { true, {
                {1, {3.2, 4.2}},
                {2, {3.5, 4.7}},
                {3, {3.6, 4.9}},
                {1, {3.2, 4.2}}
            }}
        });

    {
        std::string wkt {factory.create_multipolygon(area)};
        REQUIRE(std::string{"MULTIPOLYGON(((3.2 4.2,3.5 4.7,3.6 4.9,3.2 4.2)))"} == wkt);
    }
}

SECTION("area_1outer_1inner") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    osmium::Area& area = buffer_add_area(buffer,
        "foo",
        {},
        {
            { true, {
                {1, {0.1, 0.1}},
                {2, {9.1, 0.1}},
                {3, {9.1, 9.1}},
                {4, {0.1, 9.1}},
                {1, {0.1, 0.1}}
            }},
            { false, {
                {5, {1.0, 1.0}},
                {6, {8.0, 1.0}},
                {7, {8.0, 8.0}},
                {8, {1.0, 8.0}},
                {5, {1.0, 1.0}}
            }}
        });

    {
        std::string wkt {factory.create_multipolygon(area)};
        REQUIRE(std::string{"MULTIPOLYGON(((0.1 0.1,9.1 0.1,9.1 9.1,0.1 9.1,0.1 0.1),(1 1,8 1,8 8,1 8,1 1)))"} == wkt);
    }
}

SECTION("area_2outer_2inner") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    osmium::Area& area = buffer_add_area(buffer,
        "foo",
        {},
        {
            { true, {
                {1, {0.1, 0.1}},
                {2, {9.1, 0.1}},
                {3, {9.1, 9.1}},
                {4, {0.1, 9.1}},
                {1, {0.1, 0.1}}
            }},
            { false, {
                {5, {1.0, 1.0}},
                {6, {4.0, 1.0}},
                {7, {4.0, 4.0}},
                {8, {1.0, 4.0}},
                {5, {1.0, 1.0}}
            }},
            { false, {
                {10, {5.0, 5.0}},
                {11, {5.0, 7.0}},
                {12, {7.0, 7.0}},
                {10, {5.0, 5.0}}
            }},
            { true, {
                {100, {10.0, 10.0}},
                {101, {11.0, 10.0}},
                {102, {11.0, 11.0}},
                {103, {10.0, 11.0}},
                {100, {10.0, 10.0}}
            }}
        });

    {
        std::string wkt {factory.create_multipolygon(area)};
        REQUIRE(std::string{"MULTIPOLYGON(((0.1 0.1,9.1 0.1,9.1 9.1,0.1 9.1,0.1 0.1),(1 1,4 1,4 4,1 4,1 1),(5 5,5 7,7 7,5 5)),((10 10,11 10,11 11,10 11,10 10)))"} == wkt);
    }
}

}

