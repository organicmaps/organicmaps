#include "catch.hpp"

#include <osmium/geom/factory.hpp>
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/geom/projection.hpp>

TEST_CASE("Projection") {

SECTION("identity_projection") {
    osmium::geom::IdentityProjection projection;
    REQUIRE(4326 == projection.epsg());
    REQUIRE("+proj=longlat +datum=WGS84 +no_defs" == projection.proj_string());
}

SECTION("project_location_4326") {
    osmium::geom::Projection projection(4326);
    REQUIRE(4326 == projection.epsg());
    REQUIRE("+init=epsg:4326" == projection.proj_string());

    const osmium::Location loc(1.0, 2.0);
    const osmium::geom::Coordinates c {1.0, 2.0};
    REQUIRE(c == projection(loc));
}

SECTION("project_location_4326_string") {
    osmium::geom::Projection projection("+init=epsg:4326");
    REQUIRE(-1 == projection.epsg());
    REQUIRE("+init=epsg:4326" == projection.proj_string());

    const osmium::Location loc(1.0, 2.0);
    const osmium::geom::Coordinates c {1.0, 2.0};
    REQUIRE(c == projection(loc));
}

SECTION("unknown_projection_string") {
    REQUIRE_THROWS_AS(osmium::geom::Projection projection("abc"), osmium::projection_error);
}

SECTION("unknown_epsg_code") {
    REQUIRE_THROWS_AS(osmium::geom::Projection projection(9999999), osmium::projection_error);
}

SECTION("project_location_3857") {
    osmium::geom::Projection projection(3857);
    REQUIRE(3857 == projection.epsg());
    REQUIRE("+init=epsg:3857" == projection.proj_string());

    {
        const osmium::Location loc(0.0, 0.0);
        const osmium::geom::Coordinates c {0.0, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }
    {
        const osmium::Location loc(180.0, 0.0);
        const osmium::geom::Coordinates c {20037508.34, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }
    {
        const osmium::Location loc(180.0, 0.0);
        const osmium::geom::Coordinates c {20037508.34, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }
    {
        const osmium::Location loc(0.0, 85.0511288);
        const osmium::geom::Coordinates c {0.0, 20037508.34};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }
}

SECTION("project_location_mercator") {
    osmium::geom::MercatorProjection projection;

    {
        const osmium::Location loc(0.0, 0.0);
        const osmium::geom::Coordinates c {0.0, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }
    {
        const osmium::Location loc(180.0, 0.0);
        const osmium::geom::Coordinates c {20037508.34, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }
    {
        const osmium::Location loc(180.0, 0.0);
        const osmium::geom::Coordinates c {20037508.34, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }
    {
        const osmium::Location loc(0.0, 85.0511288);
        const osmium::geom::Coordinates c {0.0, 20037508.34};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }
}

SECTION("compare_mercators") {
    osmium::geom::MercatorProjection projection_merc;
    osmium::geom::Projection projection_3857(3857);
    REQUIRE(3857 == projection_3857.epsg());
    REQUIRE("+init=epsg:3857" == projection_3857.proj_string());

    {
        const osmium::Location loc(4.2, 27.3);
        REQUIRE(projection_merc(loc).x == Approx(projection_3857(loc).x).epsilon(0.1));
        REQUIRE(projection_merc(loc).y == Approx(projection_3857(loc).y).epsilon(0.1));
    }
    {
        const osmium::Location loc(160.789, -42.42);
        REQUIRE(projection_merc(loc).x == Approx(projection_3857(loc).x).epsilon(0.1));
        REQUIRE(projection_merc(loc).y == Approx(projection_3857(loc).y).epsilon(0.1));
    }
    {
        const osmium::Location loc(-0.001, 0.001);
        REQUIRE(projection_merc(loc).x == Approx(projection_3857(loc).x).epsilon(0.1));
        REQUIRE(projection_merc(loc).y == Approx(projection_3857(loc).y).epsilon(0.1));
    }
    {
        const osmium::Location loc(-85.2, -85.2);
        REQUIRE(projection_merc(loc).x == Approx(projection_3857(loc).x).epsilon(0.1));
        REQUIRE(projection_merc(loc).y == Approx(projection_3857(loc).y).epsilon(0.1));
    }
}

}
