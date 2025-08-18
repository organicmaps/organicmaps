#include "catch.hpp"

#include <osmium/builder/builder_helper.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/tag.hpp>

TEST_CASE("tag_list") {

    SECTION("can_be_created_from_initializer_list") {
        osmium::memory::Buffer buffer(10240);

        const osmium::TagList& tl = osmium::builder::build_tag_list(buffer, {
            { "highway", "primary" },
            { "name", "Main Street" },
            { "source", "GPS" }
        });

        REQUIRE(osmium::item_type::tag_list == tl.type());
        REQUIRE(3 == tl.size());
        REQUIRE(std::string("highway") == tl.begin()->key());
        REQUIRE(std::string("primary") == tl.begin()->value());
    }

    SECTION("can_be_created_from_map") {
        osmium::memory::Buffer buffer(10240);

        const osmium::TagList& tl = osmium::builder::build_tag_list_from_map(buffer, std::map<const char*, const char*>({
            { "highway", "primary" },
            { "name", "Main Street" }
        }));

        REQUIRE(osmium::item_type::tag_list == tl.type());
        REQUIRE(2 == tl.size());

        if (std::string("highway") == tl.begin()->key()) {
            REQUIRE(std::string("primary") == tl.begin()->value());
            REQUIRE(std::string("name") == std::next(tl.begin(), 1)->key());
            REQUIRE(std::string("Main Street") == std::next(tl.begin(), 1)->value());
        } else {
            REQUIRE(std::string("highway") == std::next(tl.begin(), 1)->key());
            REQUIRE(std::string("primary") == std::next(tl.begin(), 1)->value());
            REQUIRE(std::string("name") == tl.begin()->key());
            REQUIRE(std::string("Main Street") == tl.begin()->value());
        }
    }

    SECTION("can_be_created_with_callback") {
        osmium::memory::Buffer buffer(10240);

        const osmium::TagList& tl = osmium::builder::build_tag_list_from_func(buffer, [](osmium::builder::TagListBuilder& tlb) {
            tlb.add_tag("highway", "primary");
            tlb.add_tag("bridge", "true");
        });

        REQUIRE(osmium::item_type::tag_list == tl.type());
        REQUIRE(2 == tl.size());
        REQUIRE(std::string("bridge") == std::next(tl.begin(), 1)->key());
        REQUIRE(std::string("true") == std::next(tl.begin(), 1)->value());
    }

    SECTION("returns_value_by_key") {
        osmium::memory::Buffer buffer(10240);

        const osmium::TagList& tl = osmium::builder::build_tag_list_from_func(buffer, [](osmium::builder::TagListBuilder& tlb) {
            tlb.add_tag("highway", "primary");
            tlb.add_tag("bridge", "true");
        });

        REQUIRE(std::string("primary") == tl.get_value_by_key("highway"));
        REQUIRE(nullptr == tl.get_value_by_key("name"));
        REQUIRE(std::string("foo") == tl.get_value_by_key("name", "foo"));

        REQUIRE(std::string("true") == tl["bridge"]);
    }

}
