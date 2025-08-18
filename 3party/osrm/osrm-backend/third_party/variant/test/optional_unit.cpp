#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "optional.hpp"

using namespace mapbox;

struct dummy {
    dummy(int _m_1, int _m_2) : m_1(_m_1), m_2(_m_2) {}
    int m_1;
    int m_2;

};

int main (int argc, char* const argv[])
{
    int result = Catch::Session().run(argc, argv);
    if (!result) printf("\x1b[1;32m ✓ \x1b[0m\n");
    return result;
}

TEST_CASE( "optional can be instantiated with a POD type", "[optiona]" ) {
    mapbox::util::optional<double> dbl_opt;

    REQUIRE(!dbl_opt);
    dbl_opt = 3.1415;
    REQUIRE(dbl_opt);

    REQUIRE(dbl_opt.get() == 3.1415);
    REQUIRE(*dbl_opt == 3.1415);
}

TEST_CASE( "copy c'tor", "[optiona]" ) {
    mapbox::util::optional<double> dbl_opt;

    REQUIRE(!dbl_opt);
    dbl_opt = 3.1415;
    REQUIRE(dbl_opt);

    mapbox::util::optional<double> other = dbl_opt;

    REQUIRE(other.get() == 3.1415);
    REQUIRE(*other == 3.1415);
}

TEST_CASE( "const operator*, const get()", "[optiona]" ) {
    mapbox::util::optional<double> dbl_opt = 3.1415;

    REQUIRE(dbl_opt);

    const double pi1 = dbl_opt.get();
    const double pi2 = *dbl_opt;

    REQUIRE(pi1 == 3.1415);
    REQUIRE(pi2 == 3.1415);
}

TEST_CASE( "emplace initialization, reset", "[optional]" ) {
    mapbox::util::optional<dummy> dummy_opt;
    REQUIRE(!dummy_opt);

    // rvalues, baby!
    dummy_opt.emplace(1, 2);
    REQUIRE(dummy_opt);
    REQUIRE(dummy_opt.get().m_1 == 1);
    REQUIRE((*dummy_opt).m_2 == 2);

    dummy_opt.reset();
    REQUIRE(!dummy_opt);
}

TEST_CASE( "assignment", "[optional]") {
    mapbox::util::optional<int> a;
    mapbox::util::optional<int> b;

    a = 1; b = 3;
    REQUIRE(a.get() == 1);
    REQUIRE(b.get() == 3);
    b = a;
    REQUIRE(a.get() == b.get());
    REQUIRE(b.get() == 1);
}
