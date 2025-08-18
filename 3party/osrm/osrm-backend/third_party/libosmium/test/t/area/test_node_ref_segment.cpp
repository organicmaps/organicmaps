#include "catch.hpp"

#include <osmium/area/detail/node_ref_segment.hpp>

using osmium::area::detail::NodeRefSegment;

TEST_CASE("NodeRefSegmentClass") {

    SECTION("instantiation_with_default_parameters") {
        NodeRefSegment s;
        REQUIRE(s.first().ref() == 0);
        REQUIRE(s.first().location() == osmium::Location());
        REQUIRE(s.second().ref() == 0);
        REQUIRE(s.second().location() == osmium::Location());
    }

    SECTION("instantiation") {
        osmium::NodeRef nr1(1, { 1.2, 3.4 });
        osmium::NodeRef nr2(2, { 1.4, 3.1 });
        osmium::NodeRef nr3(3, { 1.2, 3.6 });
        osmium::NodeRef nr4(4, { 1.2, 3.7 });

        NodeRefSegment s1(nr1, nr2, nullptr, nullptr);
        REQUIRE(s1.first().ref() == 1);
        REQUIRE(s1.second().ref() == 2);

        NodeRefSegment s2(nr2, nr3, nullptr, nullptr);
        REQUIRE(s2.first().ref() == 3);
        REQUIRE(s2.second().ref() == 2);

        NodeRefSegment s3(nr3, nr4, nullptr, nullptr);
        REQUIRE(s3.first().ref() == 3);
        REQUIRE(s3.second().ref() == 4);
    }

    SECTION("intersection") {
        NodeRefSegment s1({ 1, {0.0, 0.0}}, { 2, {2.0, 2.0}}, nullptr, nullptr);
        NodeRefSegment s2({ 3, {0.0, 2.0}}, { 4, {2.0, 0.0}}, nullptr, nullptr);
        NodeRefSegment s3({ 5, {2.0, 0.0}}, { 6, {4.0, 2.0}}, nullptr, nullptr);
        NodeRefSegment s4({ 7, {1.0, 0.0}}, { 8, {3.0, 2.0}}, nullptr, nullptr);
        NodeRefSegment s5({ 9, {0.0, 4.0}}, {10, {4.0, 0.0}}, nullptr, nullptr);
        NodeRefSegment s6({11, {0.0, 0.0}}, {12, {1.0, 1.0}}, nullptr, nullptr);
        NodeRefSegment s7({13, {1.0, 1.0}}, {14, {3.0, 3.0}}, nullptr, nullptr);

        REQUIRE(calculate_intersection(s1, s2) == osmium::Location(1.0, 1.0));
        REQUIRE(calculate_intersection(s1, s3) == osmium::Location());
        REQUIRE(calculate_intersection(s2, s3) == osmium::Location());
        REQUIRE(calculate_intersection(s1, s4) == osmium::Location());
        REQUIRE(calculate_intersection(s1, s5) == osmium::Location(2.0, 2.0));
        REQUIRE(calculate_intersection(s1, s1) == osmium::Location());
        REQUIRE(calculate_intersection(s1, s6) == osmium::Location());
        REQUIRE(calculate_intersection(s1, s7) == osmium::Location());
    }

    SECTION("to_left_of") {
        osmium::Location loc { 2.0, 2.0 };

        REQUIRE(NodeRefSegment({0, {0.0, 0.0}}, {1, {0.0, 4.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {4.0, 0.0}}, {1, {4.0, 4.0}}, nullptr, nullptr).to_left_of(loc) == false);
        REQUIRE(NodeRefSegment({0, {1.0, 0.0}}, {1, {1.0, 4.0}}, nullptr, nullptr).to_left_of(loc));

        REQUIRE(NodeRefSegment({0, {0.0, 0.0}}, {1, {1.0, 4.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {0.0, 0.0}}, {1, {2.0, 4.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {0.0, 0.0}}, {1, {3.0, 4.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {0.0, 0.0}}, {1, {4.0, 4.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {0.0, 0.0}}, {1, {4.0, 3.0}}, nullptr, nullptr).to_left_of(loc) == false);

        REQUIRE(NodeRefSegment({0, {1.0, 3.0}}, {1, {2.0, 0.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {1.0, 3.0}}, {1, {3.0, 1.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {1.0, 3.0}}, {1, {3.0, 2.0}}, nullptr, nullptr).to_left_of(loc) == false);

        REQUIRE(NodeRefSegment({0, {0.0, 2.0}}, {1, {2.0, 2.0}}, nullptr, nullptr).to_left_of(loc) == false);

        REQUIRE(NodeRefSegment({0, {2.0, 0.0}}, {1, {2.0, 4.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {2.0, 0.0}}, {1, {2.0, 2.0}}, nullptr, nullptr).to_left_of(loc) == false);
        REQUIRE(NodeRefSegment({0, {2.0, 2.0}}, {1, {2.0, 4.0}}, nullptr, nullptr).to_left_of(loc) == false);

        REQUIRE(NodeRefSegment({0, {0.0, 0.0}}, {1, {0.0, 1.0}}, nullptr, nullptr).to_left_of(loc) == false);
        REQUIRE(NodeRefSegment({0, {1.0, 0.0}}, {1, {0.0, 1.0}}, nullptr, nullptr).to_left_of(loc) == false);

        REQUIRE(NodeRefSegment({0, {0.0, 0.0}}, {1, {1.0, 3.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {0.0, 2.0}}, {1, {2.0, 0.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {0.0, 2.0}}, {1, {3.0, 4.0}}, nullptr, nullptr).to_left_of(loc) == false);

        REQUIRE(NodeRefSegment({0, {1.0, 0.0}}, {1, {1.0, 2.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {0.0, 2.0}}, {1, {1.0, 2.0}}, nullptr, nullptr).to_left_of(loc) == false);
        REQUIRE(NodeRefSegment({0, {0.0, 2.0}}, {1, {1.0, 4.0}}, nullptr, nullptr).to_left_of(loc) == false);

        REQUIRE(NodeRefSegment({0, {0.0, 0.0}}, {1, {0.0, 2.0}}, nullptr, nullptr).to_left_of(loc));
        REQUIRE(NodeRefSegment({0, {0.0, 2.0}}, {1, {4.0, 4.0}}, nullptr, nullptr).to_left_of(loc) == false);

        REQUIRE(NodeRefSegment({0, {0.0, 1.0}}, {1, {2.0, 2.0}}, nullptr, nullptr).to_left_of(loc) == false);
        REQUIRE(NodeRefSegment({0, {2.0, 2.0}}, {1, {4.0, 0.0}}, nullptr, nullptr).to_left_of(loc) == false);
    }

    SECTION("ordering") {
        osmium::NodeRef node_ref1(1, { 1.0, 3.0 });
        osmium::NodeRef node_ref2(2, { 1.4, 2.9 });
        osmium::NodeRef node_ref3(3, { 1.2, 3.0 });
        osmium::NodeRef node_ref4(4, { 1.2, 3.3 });

        REQUIRE(node_ref1 < node_ref2);
        REQUIRE(node_ref2 < node_ref3);
        REQUIRE(node_ref1 < node_ref3);
        REQUIRE(node_ref1 >= node_ref1);

        REQUIRE( osmium::location_less()(node_ref1, node_ref2));
        REQUIRE(!osmium::location_less()(node_ref2, node_ref3));
        REQUIRE( osmium::location_less()(node_ref1, node_ref3));
        REQUIRE( osmium::location_less()(node_ref3, node_ref4));
        REQUIRE(!osmium::location_less()(node_ref1, node_ref1));
    }

}

