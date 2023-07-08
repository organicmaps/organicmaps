#include "catch.hpp"

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/osm/way.hpp>

#include "helper.hpp"

TEST_CASE("Basic_Way") {

SECTION("way_builder") {
    osmium::memory::Buffer buffer(10000);

    osmium::Way& way = buffer_add_way(buffer,
        "foo",
        {{"highway", "residential"}, {"name", "High Street"}},
        {1, 3, 2});

    way.set_id(17)
        .set_version(3)
        .set_visible(true)
        .set_changeset(333)
        .set_uid(21)
        .set_timestamp(123);

    REQUIRE(osmium::item_type::way == way.type());
    REQUIRE(way.type_is_in(osmium::osm_entity_bits::way));
    REQUIRE(way.type_is_in(osmium::osm_entity_bits::node | osmium::osm_entity_bits::way));
    REQUIRE(17 == way.id());
    REQUIRE(3 == way.version());
    REQUIRE(true == way.visible());
    REQUIRE(333 == way.changeset());
    REQUIRE(21 == way.uid());
    REQUIRE(std::string("foo") == way.user());
    REQUIRE(123 == way.timestamp());
    REQUIRE(2 == way.tags().size());
    REQUIRE(3 == way.nodes().size());
    REQUIRE(1 == way.nodes()[0].ref());
    REQUIRE(3 == way.nodes()[1].ref());
    REQUIRE(2 == way.nodes()[2].ref());
    REQUIRE(! way.is_closed());
}

SECTION("closed_way") {
    osmium::memory::Buffer buffer(10000);

    osmium::Way& way = buffer_add_way(buffer,
        "foo",
        {{"highway", "residential"}, {"name", "High Street"}},
        {1, 3, 1});

    REQUIRE(way.is_closed());
}

SECTION("way_builder_with_helpers") {
    osmium::memory::Buffer buffer(10000);
    {
        osmium::builder::WayBuilder builder(buffer);
        builder.add_user("username");
        builder.add_tags({
            {"amenity", "restaurant"},
            {"name", "Zum goldenen Schwanen"}
        });
        builder.add_node_refs({
            {22, {3.5, 4.7}},
            {67, {4.1, 2.2}}
        });
    }
    buffer.commit();
    osmium::Way& way = buffer.get<osmium::Way>(0);

    REQUIRE(std::string("username") == way.user());

    REQUIRE(2 == way.tags().size());
    REQUIRE(std::string("amenity") == way.tags().begin()->key());
    REQUIRE(std::string("Zum goldenen Schwanen") == way.tags()["name"]);

    REQUIRE(2 == way.nodes().size());
    REQUIRE(22 == way.nodes()[0].ref());
    REQUIRE(4.1 == way.nodes()[1].location().lon());
}

}
