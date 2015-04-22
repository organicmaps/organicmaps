#include "catch.hpp"

#include <osmium/osm/relation.hpp>

#include "helper.hpp"

TEST_CASE("Basic_Relation") {

SECTION("relation_builder") {
    osmium::memory::Buffer buffer(10000);

    osmium::Relation& relation = buffer_add_relation(buffer,
        "foo", {
            {"type", "multipolygon"},
            {"name", "Sherwood Forest"}
        }, {
            std::make_tuple('w', 1, "inner"),
            std::make_tuple('w', 2, ""),
            std::make_tuple('w', 3, "outer")
        });

    relation.set_id(17)
        .set_version(3)
        .set_visible(true)
        .set_changeset(333)
        .set_uid(21)
        .set_timestamp(123);

    REQUIRE(17 == relation.id());
    REQUIRE(3 == relation.version());
    REQUIRE(true == relation.visible());
    REQUIRE(333 == relation.changeset());
    REQUIRE(21 == relation.uid());
    REQUIRE(std::string("foo") == relation.user());
    REQUIRE(123 == relation.timestamp());
    REQUIRE(2 == relation.tags().size());
    REQUIRE(3 == relation.members().size());

    int n=1;
    for (auto& member : relation.members()) {
        REQUIRE(osmium::item_type::way == member.type());
        REQUIRE(n == member.ref());
        switch (n) {
            case 1:
                REQUIRE(std::string("inner") == member.role());
                break;
            case 2:
                REQUIRE(std::string("") == member.role());
                break;
            case 3:
                REQUIRE(std::string("outer") == member.role());
                break;
            default:
                REQUIRE(false);
        }
        ++n;
    }
}

}
