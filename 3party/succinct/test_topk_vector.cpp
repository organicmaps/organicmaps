#define BOOST_TEST_MODULE topk_vector
#include "test_common.hpp"

#include <cstdlib>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple_io.hpp>

#include "mapper.hpp"
#include "mappable_vector.hpp"
#include "topk_vector.hpp"
#include "elias_fano_compressed_list.hpp"

typedef uint64_t value_type;

// XXX test (de)serialization

struct value_index_comparator {
    template <typename Tuple>
    bool operator()(Tuple const& a, Tuple const& b) const
    {
        using boost::get;
        // lexicographic, decreasing on value and increasing
        // on index
        return (get<0>(a) > get<0>(b) ||
                (get<0>(a) == get<0>(b) &&
                 get<1>(a) < get<1>(b)));
    }
};

template <typename TopKVector>
void test_topk(std::vector<value_type> const& v, TopKVector const& topkv, std::string /* test_name */)
{
    BOOST_REQUIRE_EQUAL(v.size(), topkv.size());

    if (v.empty()) return;

    // test random pairs
    const size_t sample_size = 100;
    typedef std::pair<uint64_t, uint64_t> range_pair;
    std::vector<range_pair> pairs_sample;
    for (size_t i = 0; i < sample_size; ++i) {
        uint64_t a = size_t(rand()) % v.size();
        uint64_t b = a + (size_t(rand()) % (v.size() - a));
        pairs_sample.push_back(range_pair(a, b));
    }

    typedef typename TopKVector::entry_type entry_type;

    size_t k = 10;

    for (size_t i = 0; i < pairs_sample.size(); ++i) {
        range_pair r = pairs_sample[i];
        uint64_t a = r.first, b = r.second;

        std::vector<entry_type> expected;
        for (uint64_t i = a; i <= b; ++i) {
            expected.push_back(entry_type(v[i], i));
        }
        std::sort(expected.begin(), expected.end(), value_index_comparator()); // XXX
        expected.resize(std::min(expected.size(), k));

        std::vector<entry_type> found = topkv.topk(a, b, k);

        BOOST_REQUIRE_EQUAL_COLLECTIONS(expected.begin(), expected.end(),
                                        found.begin(), found.end());
    }
}

BOOST_AUTO_TEST_CASE(topk_vector)
{
    srand(42);

    //typedef succinct::topk_vector<succinct::mapper::mappable_vector<uint64_t> > topk_type;
    typedef succinct::topk_vector<succinct::elias_fano_compressed_list> topk_type;

    {
        std::vector<value_type> v;
        topk_type t(v);
        test_topk(v, t, "Empty vector");
    }

    {
        std::vector<value_type> v(20000);
        for (size_t i = 0; i < v.size(); ++i) {
            if (i < v.size() / 2) {
                v[i] = i;
            } else {
                v[i] = v.size() - i;
            }
        }

        {
            topk_type t(v);
            test_topk(v, t, "Convex values");
        }
    }

    {
        size_t sizes[] = {2, 4, 512, 514, 8190, 8192, 8194, 16384, 16386, 100000};
        for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
            std::vector<value_type> v(sizes[i]);
            for (size_t i = 0; i < v.size(); ++i) {
                v[i] = size_t(rand()) % 1024;
            }

            topk_type t(v);
            test_topk(v, t, "Random values");
        }
    }
}
