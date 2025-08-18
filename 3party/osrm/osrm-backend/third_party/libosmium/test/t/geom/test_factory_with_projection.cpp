#include "catch.hpp"

#include <osmium/geom/geos.hpp>
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/geom/projection.hpp>
#include <osmium/geom/wkb.hpp>
#include <osmium/geom/wkt.hpp>

#include "helper.hpp"

TEST_CASE("Projection") {

    SECTION("point_mercator") {
        osmium::geom::WKTFactory<osmium::geom::MercatorProjection> factory(2);

        std::string wkt {factory.create_point(osmium::Location(3.2, 4.2))};
        REQUIRE(std::string {"POINT(356222.37 467961.14)"} == wkt);
    }

    SECTION("point_epsg_3857") {
        osmium::geom::WKTFactory<osmium::geom::Projection> factory(osmium::geom::Projection(3857), 2);

        std::string wkt {factory.create_point(osmium::Location(3.2, 4.2))};
        REQUIRE(std::string {"POINT(356222.37 467961.14)"} == wkt);
    }

    SECTION("wkb_with_parameter") {
        osmium::geom::WKBFactory<osmium::geom::Projection> wkb_factory(osmium::geom::Projection(3857), osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);
        osmium::geom::GEOSFactory<osmium::geom::Projection> geos_factory(osmium::geom::Projection(3857));

        std::string wkb = wkb_factory.create_point(osmium::Location(3.2, 4.2));
        std::unique_ptr<geos::geom::Point> geos_point = geos_factory.create_point(osmium::Location(3.2, 4.2));
        REQUIRE(geos_to_wkb(geos_point.get()) == wkb);
    }

    SECTION("cleanup") {
        // trying to make valgrind happy, but there is still a memory leak in proj library
        pj_deallocate_grids();
    }

}
