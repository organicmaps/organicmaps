#define BOOST_TEST_MODULE rs_bit_vector
#include "test_common.hpp"
#include "test_rank_select_common.hpp"

#include <cstdlib>
#include <boost/foreach.hpp>

#include "mapper.hpp"
#include "rs_bit_vector.hpp"

BOOST_AUTO_TEST_CASE(rs_bit_vector)
{
    srand(42);

    // empty vector
    std::vector<bool> v;
    succinct::rs_bit_vector bitmap;

    succinct::rs_bit_vector(v).swap(bitmap);
    BOOST_REQUIRE_EQUAL(v.size(), bitmap.size());
    succinct::rs_bit_vector(v, true).swap(bitmap);
    BOOST_REQUIRE_EQUAL(v.size(), bitmap.size());

    // random vector
    v = random_bit_vector();

    succinct::rs_bit_vector(v).swap(bitmap);
    BOOST_REQUIRE_EQUAL(v.size(), bitmap.size());
    test_equal_bits(v, bitmap, "RS - Uniform bits");
    test_rank_select(v, bitmap, "Uniform bits");

    succinct::rs_bit_vector(v, true, true).swap(bitmap);
    test_rank_select(v, bitmap, "Uniform bits - with hints");

    v.resize(10000);
    v[9999] = 1;
    v[9000] = 1;
    succinct::rs_bit_vector(v).swap(bitmap);

    BOOST_REQUIRE_EQUAL(v.size(), bitmap.size());
    test_rank_select(v, bitmap, "Long runs of 0");
    succinct::rs_bit_vector(v, true, true).swap(bitmap);
    test_rank_select(v, bitmap, "Long runs of 0 - with hints");

    // corner cases
    v.clear();
    v.resize(10000);
    v[0] = 1;
    v[511] = 1;
    v[512] = 1;
    v[1024] = 1;
    v[2112] = 1;
    succinct::rs_bit_vector(v).swap(bitmap);

    BOOST_REQUIRE_EQUAL(v.size(), bitmap.size());
    test_rank_select(v, bitmap, "Corner cases");
    succinct::rs_bit_vector(v, true).swap(bitmap);
    test_rank_select(v, bitmap, "Corner cases - with hints");
}
