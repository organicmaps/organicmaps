
#include "common.hpp"

class TestHandler110 : public osmium::handler::Handler {

public:

    TestHandler110() :
        osmium::handler::Handler() {
    }

    void node(const osmium::Node& node) {
        if (node.id() == 110000) {
            REQUIRE(node.location().lon() == 1.02);
            REQUIRE(node.location().lat() == 1.12);
        } else if (node.id() == 110001) {
            REQUIRE(node.location().lon() == 1.07);
            REQUIRE(node.location().lat() == 1.13);
        } else {
            throw std::runtime_error("Unknown ID");
        }
    }

    void way(const osmium::Way& way) {
        if (way.id() == 110800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 2);
            REQUIRE(!way.is_closed());

            const char *test_id = way.tags().get_value_by_key("test:id");
            REQUIRE(test_id);
            REQUIRE(!strcmp(test_id, "110"));
        } else {
            throw std::runtime_error("Unknown ID");
        }
    }

}; // class TestHandler110

TEST_CASE("110") {

    SECTION("test 110") {
        osmium::io::Reader reader(dirname + "/1/110/data.osm");

        index_pos_type index_pos;
        index_neg_type index_neg;
        location_handler_type location_handler(index_pos, index_neg);
        location_handler.ignore_errors();

        CheckBasicsHandler check_basics_handler(110, 2, 1, 0);
        CheckWKTHandler check_wkt_handler(dirname, 110);
        TestHandler110 test_handler;

        osmium::apply(reader, location_handler, check_basics_handler, check_wkt_handler, test_handler);
    }

}

