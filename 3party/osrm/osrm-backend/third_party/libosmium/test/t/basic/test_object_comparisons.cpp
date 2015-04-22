#include "catch.hpp"

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/object_comparisons.hpp>

TEST_CASE("Object_Comparisons") {

    SECTION("order") {
        osmium::memory::Buffer buffer(10 * 1000);

        {
            // add node 1
            osmium::builder::NodeBuilder node_builder(buffer);
            node_builder.add_user("testuser");
            buffer.commit();
        }

        {
            // add node 2
            osmium::builder::NodeBuilder node_builder(buffer);
            node_builder.add_user("testuser");
            buffer.commit();
        }

        auto it = buffer.begin();
        osmium::Node& node1 = static_cast<osmium::Node&>(*it);
        osmium::Node& node2 = static_cast<osmium::Node&>(*(++it));

        node1.set_id(10);
        node1.set_version(1);
        node2.set_id(15);
        node2.set_version(2);
        REQUIRE(true == (node1 < node2));
        REQUIRE(false == (node1 > node2));
        node1.set_id(20);
        node1.set_version(1);
        node2.set_id(20);
        node2.set_version(2);
        REQUIRE(true == (node1 < node2));
        REQUIRE(false == (node1 > node2));
        node1.set_id(-10);
        node1.set_version(2);
        node2.set_id(-15);
        node2.set_version(1);
        REQUIRE(true == (node1 < node2));
        REQUIRE(false == (node1 > node2));
    }

    SECTION("order_types") {
        osmium::memory::Buffer buffer(10 * 1000);

        {
            // add node 1
            osmium::builder::NodeBuilder node_builder(buffer);
            osmium::Node& node = node_builder.object();
            REQUIRE(osmium::item_type::node == node.type());

            node.set_id(3);
            node.set_version(3);
            node_builder.add_user("testuser");

            buffer.commit();
        }

        {
            // add node 2
            osmium::builder::NodeBuilder node_builder(buffer);
            osmium::Node& node = node_builder.object();
            REQUIRE(osmium::item_type::node == node.type());

            node.set_id(3);
            node.set_version(4);
            node_builder.add_user("testuser");

            buffer.commit();
        }

        {
            // add node 3
            osmium::builder::NodeBuilder node_builder(buffer);
            osmium::Node& node = node_builder.object();
            REQUIRE(osmium::item_type::node == node.type());

            node.set_id(3);
            node.set_version(4);
            node_builder.add_user("testuser");

            buffer.commit();
        }

        {
            // add way
            osmium::builder::WayBuilder way_builder(buffer);
            osmium::Way& way = way_builder.object();
            REQUIRE(osmium::item_type::way == way.type());

            way.set_id(2);
            way.set_version(2);
            way_builder.add_user("testuser");

            buffer.commit();
        }

        {
            // add relation
            osmium::builder::RelationBuilder relation_builder(buffer);
            osmium::Relation& relation = relation_builder.object();
            REQUIRE(osmium::item_type::relation == relation.type());

            relation.set_id(1);
            relation.set_version(1);
            relation_builder.add_user("testuser");

            buffer.commit();
        }

        auto it = buffer.begin();
        const osmium::Node& node1 = static_cast<const osmium::Node&>(*it);
        const osmium::Node& node2 = static_cast<const osmium::Node&>(*(++it));
        const osmium::Node& node3 = static_cast<const osmium::Node&>(*(++it));
        const osmium::Way& way = static_cast<const osmium::Way&>(*(++it));
        const osmium::Relation& relation = static_cast<const osmium::Relation&>(*(++it));

        REQUIRE(true == (node1 < node2));
        REQUIRE(true == (node2 < way));
        REQUIRE(false == (node2 > way));
        REQUIRE(true == (way < relation));
        REQUIRE(true == (node1 < relation));

        REQUIRE(true == osmium::object_order_type_id_version()(node1, node2));
        REQUIRE(true == osmium::object_order_type_id_reverse_version()(node2, node1));
        REQUIRE(true == osmium::object_order_type_id_version()(node1, way));
        REQUIRE(true == osmium::object_order_type_id_reverse_version()(node1, way));

        REQUIRE(false == osmium::object_equal_type_id_version()(node1, node2));
        REQUIRE(true == osmium::object_equal_type_id_version()(node2, node3));

        REQUIRE(true == osmium::object_equal_type_id()(node1, node2));
        REQUIRE(true == osmium::object_equal_type_id()(node2, node3));

        REQUIRE(false == osmium::object_equal_type_id_version()(node1, way));
        REQUIRE(false == osmium::object_equal_type_id_version()(node1, relation));
        REQUIRE(false == osmium::object_equal_type_id()(node1, relation));
    }

}
