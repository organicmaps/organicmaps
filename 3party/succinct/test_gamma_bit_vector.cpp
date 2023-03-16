#define BOOST_TEST_MODULE gamma_bit_vector
#include "test_common.hpp"

#include <cstdlib>

#include "gamma_bit_vector.hpp"

typedef std::vector<uint64_t> std_vector_type;

std_vector_type random_vector(size_t test_size)
{
    std_vector_type v;

    for (size_t i = 0; i < test_size; ++i) {
        bool b = uint64_t(rand()) & 1;
        if (rand() < (RAND_MAX / 3)) {
            v.push_back(b);
        } else {
            v.push_back((uint64_t(rand()) << 1) | b);
        }
    }

    return v;
}

BOOST_AUTO_TEST_CASE(gamma_bit_vector)
{
    srand(42);
    const size_t test_size = 12345;
    std_vector_type v = random_vector(test_size);

    succinct::gamma_bit_vector vv(v);

    BOOST_REQUIRE_EQUAL(v.size(), vv.size());
    for (size_t i = 0; i < v.size(); ++i) {
        MY_REQUIRE_EQUAL(v[i], vv[i], "i = " << i);
    }
}

BOOST_AUTO_TEST_CASE(gamma_bit_enumerator)
{
    srand(42);
    const size_t test_size = 12345;
    std_vector_type v = random_vector(test_size);

    succinct::gamma_bit_vector vv(v);

    size_t i = 0;
    size_t pos = 0;

    succinct::forward_enumerator<succinct::gamma_bit_vector> e(vv, pos);
    while (pos < vv.size()) {
        succinct::gamma_bit_vector::value_type next = e.next();
        MY_REQUIRE_EQUAL(next, v[pos], "pos = " << pos << " i = " << i);
        pos += 1;

        size_t step = uint64_t(rand()) % (vv.size() - pos + 1);
        pos += step;
        e = succinct::forward_enumerator<succinct::gamma_bit_vector>(vv, pos);
        i += 1;
    }
}
