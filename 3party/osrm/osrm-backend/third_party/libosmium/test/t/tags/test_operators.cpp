#include "catch.hpp"

#include <iterator>

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/tag.hpp>

TEST_CASE("Operators") {

    SECTION("Equal") {
        osmium::memory::Buffer buffer1(10240);
        {
            osmium::builder::TagListBuilder tl_builder(buffer1);
            tl_builder.add_tag("highway", "primary");
            tl_builder.add_tag("name", "Main Street");
            tl_builder.add_tag("source", "GPS");
        }
        buffer1.commit();

        osmium::memory::Buffer buffer2(10240);
        {
            osmium::builder::TagListBuilder tl_builder(buffer2);
            tl_builder.add_tag("highway", "primary");
        }
        buffer2.commit();

        const osmium::TagList& tl1 = buffer1.get<const osmium::TagList>(0);
        const osmium::TagList& tl2 = buffer2.get<const osmium::TagList>(0);

        auto tagit1 = tl1.begin();
        auto tagit2 = tl2.begin();
        REQUIRE(*tagit1 == *tagit2);
        ++tagit1;
        REQUIRE(!(*tagit1 == *tagit2));
    }

    SECTION("Order") {
        osmium::memory::Buffer buffer(10240);
        {
            osmium::builder::TagListBuilder tl_builder(buffer);
            tl_builder.add_tag("highway", "residential");
            tl_builder.add_tag("highway", "primary");
            tl_builder.add_tag("name", "Main Street");
            tl_builder.add_tag("amenity", "post_box");
        }
        buffer.commit();

        const osmium::TagList& tl = buffer.get<const osmium::TagList>(0);
        const osmium::Tag& t1 = *(tl.begin());
        const osmium::Tag& t2 = *(std::next(tl.begin(), 1));
        const osmium::Tag& t3 = *(std::next(tl.begin(), 2));
        const osmium::Tag& t4 = *(std::next(tl.begin(), 3));

        REQUIRE(t2 < t1);
        REQUIRE(t1 < t3);
        REQUIRE(t2 < t3);
        REQUIRE(t4 < t1);
    }

}
