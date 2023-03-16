#include "catch.hpp"

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/osm/node.hpp>

void check_node_1(osmium::Node& node) {
    REQUIRE(1 == node.id());
    REQUIRE(3 == node.version());
    REQUIRE(true == node.visible());
    REQUIRE(333 == node.changeset());
    REQUIRE(21 == node.uid());
    REQUIRE(123 == node.timestamp());
    REQUIRE(osmium::Location(3.5, 4.7) == node.location());
    REQUIRE(std::string("testuser") == node.user());

    for (osmium::memory::Item& item : node) {
        REQUIRE(osmium::item_type::tag_list == item.type());
    }

    REQUIRE(node.tags().begin() == node.tags().end());
    REQUIRE(node.tags().empty());
    REQUIRE(0 == std::distance(node.tags().begin(), node.tags().end()));
}

void check_node_2(osmium::Node& node) {
    REQUIRE(2 == node.id());
    REQUIRE(3 == node.version());
    REQUIRE(true == node.visible());
    REQUIRE(333 == node.changeset());
    REQUIRE(21 == node.uid());
    REQUIRE(123 == node.timestamp());
    REQUIRE(osmium::Location(3.5, 4.7) == node.location());
    REQUIRE(std::string("testuser") == node.user());

    for (osmium::memory::Item& item : node) {
        REQUIRE(osmium::item_type::tag_list == item.type());
    }

    REQUIRE(!node.tags().empty());
    REQUIRE(2 == std::distance(node.tags().begin(), node.tags().end()));

    int n = 0;
    for (const osmium::Tag& tag : node.tags()) {
        switch (n) {
            case 0:
                REQUIRE(std::string("amenity") == tag.key());
                REQUIRE(std::string("bank") == tag.value());
                break;
            case 1:
                REQUIRE(std::string("name") == tag.key());
                REQUIRE(std::string("OSM Savings") == tag.value());
                break;
        }
        ++n;
    }
    REQUIRE(2 == n);
}

TEST_CASE("Buffer_Node") {

    SECTION("buffer_node") {
        constexpr size_t buffer_size = 10000;
        unsigned char data[buffer_size];

        osmium::memory::Buffer buffer(data, buffer_size, 0);

        {
            // add node 1
            osmium::builder::NodeBuilder node_builder(buffer);
            osmium::Node& node = node_builder.object();
            REQUIRE(osmium::item_type::node == node.type());

            node.set_id(1);
            node.set_version(3);
            node.set_visible(true);
            node.set_changeset(333);
            node.set_uid(21);
            node.set_timestamp(123);
            node.set_location(osmium::Location(3.5, 4.7));

            node_builder.add_user("testuser");

            buffer.commit();
        }

        {
            // add node 2
            osmium::builder::NodeBuilder node_builder(buffer);
            osmium::Node& node = node_builder.object();
            REQUIRE(osmium::item_type::node == node.type());

            node.set_id(2);
            node.set_version(3);
            node.set_visible(true);
            node.set_changeset(333);
            node.set_uid(21);
            node.set_timestamp(123);
            node.set_location(osmium::Location(3.5, 4.7));

            node_builder.add_user("testuser");

            {
                osmium::builder::TagListBuilder tag_builder(buffer, &node_builder);
                tag_builder.add_tag("amenity", "bank");
                tag_builder.add_tag("name", "OSM Savings");
            }

            buffer.commit();
        }

        REQUIRE(2 == std::distance(buffer.begin(), buffer.end()));
        int item_no = 0;
        for (osmium::memory::Item& item : buffer) {
            REQUIRE(osmium::item_type::node == item.type());

            osmium::Node& node = static_cast<osmium::Node&>(item);

            switch (item_no) {
                case 0:
                    check_node_1(node);
                    break;
                case 1:
                    check_node_2(node);
                    break;
                default:
                    break;
            }

            ++item_no;

        }

    }

}
