#include <iostream>
#include <vector>

#include <boost/lexical_cast.hpp>

#include "util.hpp"
#include "test_bp_vector_common.hpp"

#include "bp_vector.hpp"
#include "mapper.hpp"

#include "perftest_common.hpp"

// this generic trait enables easy comparisons with other BP
// implementations

struct succinct_bp_vector_traits
{
    typedef succinct::bit_vector_builder builder_type;
    typedef succinct::bp_vector bp_vector_type;

    static inline void build(builder_type& builder, bp_vector_type& bp)
    {
	bp_vector_type(&builder, true, false).swap(bp);
    }

    static inline std::string log_header()
    {
	return std::string("SUCCINCT");
    }

    static inline double bits_per_bp(bp_vector_type& vec)
    {
	return double(succinct::mapper::size_of(vec))
            * 8.0 / double(vec.size());
    }

};

template <typename BpVector>
double time_visit(BpVector const& bp, size_t sample_size = 1000000)
{
    std::vector<char> random_bits;
    for (size_t i = 0; i < sample_size; ++i) {
        random_bits.push_back(rand() > (RAND_MAX / 2));
    }

    volatile size_t foo = 0; // to prevent the compiler to optimize away the loop

    size_t find_close_performed = 0;
    size_t steps_done = 0;
    double elapsed;
    SUCCINCT_TIMEIT(elapsed) {
        while (steps_done < sample_size) {
            size_t cur_node = 1; // root

            while (bp[cur_node] && steps_done < sample_size) {
                if (random_bits[steps_done++]) {
                    size_t next_node = bp.find_close(cur_node);
		    cur_node = next_node + 1;
                    find_close_performed += 1;
                } else {
                    cur_node += 1;
                }
            }
            foo = cur_node;
        }
    }

    (void)foo; // silence warning
    return elapsed / double(find_close_performed);
}

template <typename BpVectorTraits>
void build_random_binary_tree(typename BpVectorTraits::bp_vector_type& bp, size_t size)
{
    typename BpVectorTraits::builder_type builder;
    succinct::random_binary_tree(builder, size);
    BpVectorTraits::build(builder, bp);
}

template <typename BpVectorTraits>
void bp_benchmark(size_t runs)
{
    srand(42); // make everything deterministic
    static const size_t sample_size = 10000000;

    std::cout << BpVectorTraits::log_header() << std::endl;
    std::cout << "log_height" "\t" "find_close_us" "\t" "bits_per_bp" << std::endl;

    for (size_t ln = 10; ln <= 28; ln += 2) {
	size_t n = 1 << ln;
	double elapsed = 0;
	double bits_per_bp = 0;
	for (size_t run = 0; run < runs; ++run) {
	    typename BpVectorTraits::bp_vector_type bp;
	    build_random_binary_tree<BpVectorTraits>(bp, n);
	    elapsed += time_visit(bp, sample_size);
	    bits_per_bp += BpVectorTraits::bits_per_bp(bp);
	}
	std::cout << ln
                  << "\t" << elapsed / double(runs)
                  << "\t" << bits_per_bp / double(runs)
                  << std::endl;
    }
}

int main(int argc, char** argv)
{
    size_t runs;

    if (argc == 2) {
        runs = boost::lexical_cast<size_t>(argv[1]);
    } else {
        runs = 1;
    }

    bp_benchmark<succinct_bp_vector_traits>(runs);
}
