#include "catch.hpp"

#include <osmium/builder/builder_helper.hpp>
#include <osmium/geom/geos.hpp>
#include <osmium/geom/wkb.hpp>

#include "../basic/helper.hpp"
#include "helper.hpp"

TEST_CASE("WKB_Geometry_with_GEOS") {

SECTION("point") {
    osmium::geom::WKBFactory<> wkb_factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);
    osmium::geom::GEOSFactory<> geos_factory;

    std::string wkb {wkb_factory.create_point(osmium::Location(3.2, 4.2))};

    std::unique_ptr<geos::geom::Point> geos_point = geos_factory.create_point(osmium::Location(3.2, 4.2));
    REQUIRE(geos_to_wkb(geos_point.get()) == wkb);
}


SECTION("linestring") {
    osmium::geom::WKBFactory<> wkb_factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);
    osmium::geom::GEOSFactory<> geos_factory;

    osmium::memory::Buffer buffer(10000);
    auto& wnl = osmium::builder::build_way_node_list(buffer, {
        {1, {3.2, 4.2}},
        {3, {3.5, 4.7}},
        {4, {3.5, 4.7}},
        {2, {3.6, 4.9}}
    });

    {
        std::string wkb = wkb_factory.create_linestring(wnl);
        std::unique_ptr<geos::geom::LineString> geos = geos_factory.create_linestring(wnl);
        REQUIRE(geos_to_wkb(geos.get()) == wkb);
    }

    {
        std::string wkb = wkb_factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward);
        std::unique_ptr<geos::geom::LineString> geos = geos_factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward);
        REQUIRE(geos_to_wkb(geos.get()) == wkb);
    }

    {
        std::string wkb = wkb_factory.create_linestring(wnl, osmium::geom::use_nodes::all);
        std::unique_ptr<geos::geom::LineString> geos = geos_factory.create_linestring(wnl, osmium::geom::use_nodes::all);
        REQUIRE(geos_to_wkb(geos.get()) == wkb);
    }

    {
        std::string wkb = wkb_factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward);
        std::unique_ptr<geos::geom::LineString> geos = geos_factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward);
        REQUIRE(geos_to_wkb(geos.get()) == wkb);
    }
}

SECTION("area_1outer_0inner") {
    osmium::geom::WKBFactory<> wkb_factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);
    osmium::geom::GEOSFactory<> geos_factory;

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

    std::string wkb = wkb_factory.create_multipolygon(area);
    std::unique_ptr<geos::geom::MultiPolygon> geos = geos_factory.create_multipolygon(area);
    REQUIRE(geos_to_wkb(geos.get()) == wkb);
}

SECTION("area_1outer_1inner") {
    osmium::geom::WKBFactory<> wkb_factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);
    osmium::geom::GEOSFactory<> geos_factory;

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

    std::string wkb = wkb_factory.create_multipolygon(area);
    std::unique_ptr<geos::geom::MultiPolygon> geos = geos_factory.create_multipolygon(area);
    REQUIRE(geos_to_wkb(geos.get()) == wkb);
}

SECTION("area_2outer_2inner") {
    osmium::geom::WKBFactory<> wkb_factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);
    osmium::geom::GEOSFactory<> geos_factory;

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

    std::string wkb = wkb_factory.create_multipolygon(area);
    std::unique_ptr<geos::geom::MultiPolygon> geos = geos_factory.create_multipolygon(area);
    REQUIRE(geos_to_wkb(geos.get()) == wkb);
}

}

