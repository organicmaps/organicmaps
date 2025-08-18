#include "catch.hpp"

#include <osmium/osm/entity_bits.hpp>

TEST_CASE("entity_bits") {

    SECTION("can_be_set_and_checked") {
        osmium::osm_entity_bits::type entities = osmium::osm_entity_bits::node | osmium::osm_entity_bits::way;
        REQUIRE(entities == (osmium::osm_entity_bits::node | osmium::osm_entity_bits::way));

        entities |= osmium::osm_entity_bits::relation;
        REQUIRE((entities & osmium::osm_entity_bits::object));

        entities |= osmium::osm_entity_bits::area;
        REQUIRE(entities == osmium::osm_entity_bits::object);

        REQUIRE(! (entities & osmium::osm_entity_bits::changeset));

        entities &= osmium::osm_entity_bits::node;
        REQUIRE((entities & osmium::osm_entity_bits::node));
        REQUIRE(! (entities & osmium::osm_entity_bits::way));
        REQUIRE(entities == osmium::osm_entity_bits::node);

        REQUIRE(osmium::osm_entity_bits::node      == osmium::osm_entity_bits::from_item_type(osmium::item_type::node));
        REQUIRE(osmium::osm_entity_bits::way       == osmium::osm_entity_bits::from_item_type(osmium::item_type::way));
        REQUIRE(osmium::osm_entity_bits::relation  == osmium::osm_entity_bits::from_item_type(osmium::item_type::relation));
        REQUIRE(osmium::osm_entity_bits::changeset == osmium::osm_entity_bits::from_item_type(osmium::item_type::changeset));
        REQUIRE(osmium::osm_entity_bits::area      == osmium::osm_entity_bits::from_item_type(osmium::item_type::area));
    }

}
