#include "catch.hpp"

#include <osmium/geom/mercator_projection.hpp>

TEST_CASE("Mercator") {

    SECTION("mercator_projection") {
        osmium::geom::MercatorProjection projection;
        REQUIRE(3857 == projection.epsg());
        REQUIRE("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs" == projection.proj_string());
    }

    SECTION("low_level_mercator_functions") {
        osmium::geom::Coordinates c1(17.839, -3.249);
        osmium::geom::Coordinates r1 = osmium::geom::mercator_to_lonlat(osmium::geom::lonlat_to_mercator(c1));
        REQUIRE(r1.x == Approx(c1.x).epsilon(0.000001));
        REQUIRE(r1.y == Approx(c1.y).epsilon(0.000001));

        osmium::geom::Coordinates c2(-89.2, 15.915);
        osmium::geom::Coordinates r2 = osmium::geom::mercator_to_lonlat(osmium::geom::lonlat_to_mercator(c2));
        REQUIRE(r2.x == Approx(c2.x).epsilon(0.000001));
        REQUIRE(r2.y == Approx(c2.y).epsilon(0.000001));

        osmium::geom::Coordinates c3(180.0, 85.0);
        osmium::geom::Coordinates r3 = osmium::geom::mercator_to_lonlat(osmium::geom::lonlat_to_mercator(c3));
        REQUIRE(r3.x == Approx(c3.x).epsilon(0.000001));
        REQUIRE(r3.y == Approx(c3.y).epsilon(0.000001));
    }

    SECTION("mercator_bounds") {
        osmium::Location mmax(180.0, osmium::geom::MERCATOR_MAX_LAT);
        osmium::geom::Coordinates c = osmium::geom::lonlat_to_mercator(mmax);
        REQUIRE(c.x == Approx(c.y).epsilon(0.001));
        REQUIRE(osmium::geom::detail::y_to_lat(osmium::geom::detail::lon_to_x(180.0)) == Approx(osmium::geom::MERCATOR_MAX_LAT).epsilon(0.0000001));
    }

}
