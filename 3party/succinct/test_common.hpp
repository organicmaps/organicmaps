#pragma once

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <vector>
#include <stack>

#define MY_REQUIRE_EQUAL(A, B, MSG)                                     \
    BOOST_REQUIRE_MESSAGE((A) == (B), BOOST_PP_STRINGIZE(A) << " == " << BOOST_PP_STRINGIZE(B) << " [" << A  << " != " << B << "] " << MSG)

inline std::vector<bool> random_bit_vector(size_t n = 10000, double density = 0.5)
{
    std::vector<bool> v;
    for (size_t i = 0; i < n; ++i) {
        v.push_back(rand() < (RAND_MAX * density));
    }
    return v;
}
