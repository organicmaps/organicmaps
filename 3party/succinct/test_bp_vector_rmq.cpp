#define BOOST_TEST_MODULE bp_vector_rmq
#include "test_common.hpp"

#include <cstdlib>
#include <boost/foreach.hpp>

#include "mapper.hpp"
#include "bp_vector.hpp"
#include "test_bp_vector_common.hpp"


template <class BPVector>
void test_rmq(std::vector<char> const& v, BPVector const& bitmap, std::string test_name)
{
    // XXX should we test this case?
    if (v.empty()) return;

    // test all values from a to v.size() for a in specific locations
    // plus a few random

    std::vector<uint64_t> tests;
    tests.push_back(1);
    tests.push_back(8);
    tests.push_back(64);
    tests.push_back(8192);
    tests.push_back(v.size());
    for (size_t t = 0; t < 10; ++t) {
        tests.push_back(size_t(rand()) % v.size());
    }

    for(size_t t = 0; t < tests.size(); ++t) {
        uint64_t a = tests[t];
        if (a > v.size()) continue;

        typename BPVector::enumerator bp_it(bitmap, a);
        typename BPVector::excess_t cur_exc = bitmap.excess(a);
        typename BPVector::excess_t min_exc = cur_exc, found_min_exc;
        uint64_t min_idx = a;

        BOOST_REQUIRE_EQUAL(min_idx, bitmap.excess_rmq(a, a, found_min_exc));

        for (uint64_t b = a + 1; b < v.size(); ++b) {
            cur_exc += bp_it.next() ? 1 : -1;
            if (cur_exc < min_exc) {
                min_exc = cur_exc;
                min_idx = b;

                assert(min_exc == bitmap.excess(min_idx));
            }

            MY_REQUIRE_EQUAL(min_idx, bitmap.excess_rmq(a, b, found_min_exc),
                             "excess_rmq (" << test_name << "):"
                             << " a = " << a
                             << " b = " << b
                             << " min_exc = " << min_exc
                             << " found_min_exc = " << found_min_exc
                             );
        }
    }
}

BOOST_AUTO_TEST_CASE(bp_vector_rmq)
{
    srand(42);

    {
        std::vector<char> v;
        succinct::bp_vector bitmap(v);
        test_rmq(v, bitmap, "Empty vector");
    }

    {
        std::vector<char> v;
        succinct::random_bp(v, 100000);
        succinct::bp_vector bitmap(v);
        test_rmq(v, bitmap, "Random parentheses");
    }

    {
        size_t sizes[] = {2, 4, 512, 514, 8190, 8192, 8194, 16384, 16386, 100000};
        for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
            std::vector<char> v;
            succinct::random_binary_tree(v, sizes[i]);
            succinct::bp_vector bitmap(v);
            test_rmq(v, bitmap, "Random binary tree");
        }
    }

    {
        size_t sizes[] = {2, 4, 512, 514, 8190, 8192, 8194, 16384, 16386, 32768, 32770};
        size_t iterations[] = {1, 2, 3};
        for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); ++s) {
            for (size_t r = 0; r < sizeof(iterations) / sizeof(iterations[0]); ++r) {
                std::vector<char> v;
                for (size_t i = 0; i < iterations[r]; ++i) {
                    succinct::bp_path(v, sizes[s]);
                }
                succinct::bp_vector bitmap(v);
                test_rmq(v, bitmap, "Nested parentheses");
            }
        }
    }
}
