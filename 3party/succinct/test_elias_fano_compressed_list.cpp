#define BOOST_TEST_MODULE elias_fano_compressed_list
#include "test_common.hpp"

#include <cstdlib>

#include "elias_fano_compressed_list.hpp"

BOOST_AUTO_TEST_CASE(elias_fano_compressed_list)
{
    srand(42);
    const size_t test_size = 12345;

    std::vector<uint64_t> v;

    for (size_t i = 0; i < test_size; ++i) {
        if (rand() < (RAND_MAX / 3)) {
            v.push_back(0);
        } else {
            v.push_back(size_t(rand()));
        }
    }

    succinct::elias_fano_compressed_list vv(v);

    BOOST_REQUIRE_EQUAL(v.size(), vv.size());
    for (size_t i = 0; i < v.size(); ++i) {
        MY_REQUIRE_EQUAL(v[i], vv[i], "i = " << i);
    }
}
