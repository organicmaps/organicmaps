#include "catch.hpp"

#include <osmium/osm/node.hpp>

#include "helper.hpp"

TEST_CASE("Basic_Node") {

SECTION("node_builder") {
    osmium::memory::Buffer buffer(10000);

    osmium::Node& node = buffer_add_node(buffer,
        "foo",
        {{"amenity", "pub"}, {"name", "OSM BAR"}},
        {3.5, 4.7});

    node.set_id(17)
        .set_version(3)
        .set_visible(true)
        .set_changeset(333)
        .set_uid(21)
        .set_timestamp(123);

    REQUIRE(osmium::item_type::node == node.type());
    REQUIRE(node.type_is_in(osmium::osm_entity_bits::node));
    REQUIRE(node.type_is_in(osmium::osm_entity_bits::nwr));
    REQUIRE(17l == node.id());
    REQUIRE(17ul == node.positive_id());
    REQUIRE(3 == node.version());
    REQUIRE(true == node.visible());
    REQUIRE(false == node.deleted());
    REQUIRE(333 == node.changeset());
    REQUIRE(21 == node.uid());
    REQUIRE(std::string("foo") == node.user());
    REQUIRE(123 == node.timestamp());
    REQUIRE(osmium::Location(3.5, 4.7) == node.location());
    REQUIRE(2 == node.tags().size());

    node.set_visible(false);
    REQUIRE(false == node.visible());
    REQUIRE(true == node.deleted());
}

SECTION("node_default_attributes") {
    osmium::memory::Buffer buffer(10000);

    osmium::Node& node = buffer_add_node(buffer, "", {}, osmium::Location{});

    REQUIRE(0l == node.id());
    REQUIRE(0ul == node.positive_id());
    REQUIRE(0 == node.version());
    REQUIRE(true == node.visible());
    REQUIRE(0 == node.changeset());
    REQUIRE(0 == node.uid());
    REQUIRE(std::string("") == node.user());
    REQUIRE(0 == node.timestamp());
    REQUIRE(osmium::Location() == node.location());
    REQUIRE(0 == node.tags().size());
}

SECTION("set_node_attributes_from_string") {
    osmium::memory::Buffer buffer(10000);

    osmium::Node& node = buffer_add_node(buffer,
        "foo",
        {{"amenity", "pub"}, {"name", "OSM BAR"}},
        {3.5, 4.7});

    node.set_id("-17")
        .set_version("3")
        .set_visible(true)
        .set_changeset("333")
        .set_uid("21");

    REQUIRE(-17l == node.id());
    REQUIRE(17ul == node.positive_id());
    REQUIRE(3 == node.version());
    REQUIRE(true == node.visible());
    REQUIRE(333 == node.changeset());
    REQUIRE(21 == node.uid());
}

SECTION("large_id") {
    osmium::memory::Buffer buffer(10000);

    osmium::Node& node = buffer_add_node(buffer, "", {}, osmium::Location{});

    int64_t id = 3000000000l;
    node.set_id(id);

    REQUIRE(id == node.id());
    REQUIRE(static_cast<osmium::unsigned_object_id_type>(id) == node.positive_id());

    node.set_id(-id);
    REQUIRE(-id == node.id());
    REQUIRE(static_cast<osmium::unsigned_object_id_type>(id) == node.positive_id());
}

SECTION("tags") {
    osmium::memory::Buffer buffer(10000);

    osmium::Node& node = buffer_add_node(buffer,
        "foo",
        {{"amenity", "pub"}, {"name", "OSM BAR"}},
        {3.5, 4.7});

    REQUIRE(nullptr == node.tags().get_value_by_key("fail"));
    REQUIRE(std::string("pub") == node.tags().get_value_by_key("amenity"));
    REQUIRE(std::string("pub") == node.get_value_by_key("amenity"));

    REQUIRE(std::string("default") == node.tags().get_value_by_key("fail", "default"));
    REQUIRE(std::string("pub") == node.tags().get_value_by_key("amenity", "default"));
    REQUIRE(std::string("pub") == node.get_value_by_key("amenity", "default"));
}


}
