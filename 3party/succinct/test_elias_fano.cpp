#define BOOST_TEST_MODULE elias_fano
#include "test_common.hpp"
#include "test_rank_select_common.hpp"

#include <cstdlib>
#include <boost/foreach.hpp>

#include "mapper.hpp"
#include "elias_fano.hpp"

BOOST_AUTO_TEST_CASE(elias_fano)
{
    srand(42);
    size_t N = 10000;

    {
        // Random bitmap
        for (size_t d = 1; d < 8; ++d) {
            double density = 1.0 / (1 << d);
            std::vector<bool> v = random_bit_vector(N, density);

            succinct::bit_vector_builder bvb;
            for (size_t i = 0; i < v.size(); ++i) {
                bvb.push_back(v[i]);
            }

            succinct::elias_fano bitmap(&bvb);
            test_equal_bits(v, bitmap, "Random bitmap");
            test_rank_select1(v, bitmap, "Random bitmap");
            test_delta(bitmap, "Random bitmap");
            test_select_enumeration(v, bitmap, "Random bitmap");
        }
    }

    {
        // Empty bitmap
        succinct::bit_vector_builder bvb(N);
        succinct::elias_fano bitmap(&bvb);
        BOOST_REQUIRE_EQUAL(0U, bitmap.num_ones());
        test_equal_bits(std::vector<bool>(N), bitmap, "Empty bitmap");
        test_select_enumeration(std::vector<bool>(N), bitmap, "Empty bitmap");
    }

    {
        // Only one value
        std::vector<bool> v(N);
        succinct::bit_vector_builder bvb(N);
        bvb.set(37, 1);
        v[37] = 1;
        succinct::elias_fano bitmap(&bvb);
        test_equal_bits(v, bitmap, "Only one value");
        test_rank_select1(v, bitmap, "Only one value");
        test_delta(bitmap, "Only one value");
        test_select_enumeration(v, bitmap, "Only one value");
        BOOST_REQUIRE_EQUAL(1U, bitmap.num_ones());
    }

    {
        // Full bitmap
        std::vector<bool> v(N, 1);
        succinct::bit_vector_builder bvb;
        for (size_t i = 0; i < N; ++i) {
            bvb.push_back(1);
        }
        succinct::elias_fano bitmap(&bvb);
        test_equal_bits(v, bitmap, "Full bitmap");
        test_rank_select1(v, bitmap, "Full bitmap");
        test_delta(bitmap, "Full bitmap");
        test_select_enumeration(v, bitmap, "Full bitmap");
    }
}
