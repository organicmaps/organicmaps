#define BOOST_TEST_MODULE bp_vector
#include "test_common.hpp"

#include <cstdlib>
#include <boost/foreach.hpp>

#include "mapper.hpp"
#include "bp_vector.hpp"
#include "test_bp_vector_common.hpp"

template <class BPVector>
void test_parentheses(std::vector<char> const& v, BPVector const& bitmap, std::string test_name)
{
    std::stack<size_t> stack;
    std::vector<uint64_t> open(v.size());
    std::vector<uint64_t> close(v.size());
    std::vector<uint64_t> enclose(v.size(), uint64_t(-1));

    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i]) { // opening
            if (!stack.empty()) {
                enclose[i] = stack.top();
            }
            stack.push(i);
        } else { // closing
            BOOST_REQUIRE(!stack.empty()); // this is more a test on the test
            size_t opening = stack.top();
            stack.pop();
            close[opening] = i;
            open[i] = opening;

        }
    }
    BOOST_REQUIRE_EQUAL(0U, stack.size()); // ditto as above

    for (size_t i = 0; i < bitmap.size(); ++i) {
        if (v[i]) { // opening
            if (enclose[i] != uint64_t(-1)) {
                MY_REQUIRE_EQUAL(enclose[i], bitmap.enclose(i),
                                 "enclose (" << test_name << "): i = " << i);
            }
            MY_REQUIRE_EQUAL(close[i], bitmap.find_close(i),
                             "find_close (" << test_name << "): i = " << i);
        } else { // closing
            MY_REQUIRE_EQUAL(open[i], bitmap.find_open(i),
                             "find_open (" << test_name << "): i = " << i);
        }
    }
}

BOOST_AUTO_TEST_CASE(bp_vector)
{
    srand(42);

    {
        std::vector<char> v;
        succinct::bp_vector bitmap(v);
        test_parentheses(v, bitmap, "Empty vector");
    }

    {
        std::vector<char> v;
        succinct::random_bp(v, 100000);
        succinct::bp_vector bitmap(v);
        test_parentheses(v, bitmap, "Random parentheses");
    }

    {
        size_t sizes[] = {2, 4, 512, 514, 8190, 8192, 8194, 16384, 16386, 100000};
        for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
            std::vector<char> v;
            succinct::random_binary_tree(v, sizes[i]);
            succinct::bp_vector bitmap(v);
            test_parentheses(v, bitmap, "Random binary tree");
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
                test_parentheses(v, bitmap, "Nested parentheses");
            }
        }
    }
}
