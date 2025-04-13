#define BOOST_TEST_MODULE gamma_vector
#include "test_common.hpp"

#include <cstdlib>

#include "gamma_vector.hpp"

BOOST_AUTO_TEST_CASE(gamma_vector)
{
    srand(42);
    const size_t test_size = 12345;

    std::vector<uint64_t> v;

    for (size_t i = 0; i < test_size; ++i) {
        if (rand() < (RAND_MAX / 3)) {
            v.push_back(0);
        } else {
            v.push_back(uint64_t(rand()));
        }
    }

    succinct::gamma_vector vv(v);

    BOOST_REQUIRE_EQUAL(v.size(), vv.size());
    for (size_t i = 0; i < v.size(); ++i) {
        MY_REQUIRE_EQUAL(v[i], vv[i], "i = " << i);
    }
}

BOOST_AUTO_TEST_CASE(gamma_enumerator)
{
    srand(42);
    const size_t test_size = 12345;

    std::vector<uint64_t> v;

    for (size_t i = 0; i < test_size; ++i) {
        if (rand() < (RAND_MAX / 3)) {
            v.push_back(0);
        } else {
            v.push_back(uint64_t(rand()));
        }
    }

    succinct::gamma_vector vv(v);

    size_t i = 0;
    size_t pos = 0;

    succinct::forward_enumerator<succinct::gamma_vector> e(vv, pos);
    while (pos < vv.size()) {
        uint64_t next = e.next();
        MY_REQUIRE_EQUAL(next, v[pos], "pos = " << pos << " i = " << i);
        pos += 1;

        size_t step = uint64_t(rand()) % (vv.size() - pos + 1);
        pos += step;
        e = succinct::forward_enumerator<succinct::gamma_vector>(vv, pos);
        i += 1;
    }
}
