#include <iostream>
#include <vector>

#include <boost/lexical_cast.hpp>

#include "util.hpp"
#include "test_bp_vector_common.hpp"

#include "bp_vector.hpp"

#include "perftest_common.hpp"

double time_avg_rmq(succinct::bp_vector const& bp, size_t sample_size = 1000000)
{
    typedef std::pair<uint64_t, uint64_t> range_pair;
    std::vector<range_pair> pairs_sample;
    for (size_t i = 0; i < sample_size; ++i) {
        uint64_t a = uint64_t(rand()) % bp.size();
        uint64_t b = a + (uint64_t(rand()) % (bp.size() - a));
        pairs_sample.push_back(range_pair(a, b));
    }

    volatile uint64_t foo; // to prevent the compiler to optimize away the loop

    size_t rmq_performed = 0;
    double elapsed;
    SUCCINCT_TIMEIT(elapsed) {
        for (size_t i = 0; i < pairs_sample.size(); ++i) {
            range_pair r = pairs_sample[i];
            foo = bp.excess_rmq(r.first, r.second);
            rmq_performed += 1;
        }
    }

    (void)foo; // silence warning
    return elapsed / double(rmq_performed);
}

void build_random_binary_tree(succinct::bp_vector& bp, size_t size)
{
    succinct::bit_vector_builder builder;
    succinct::random_binary_tree(builder, size);
    succinct::bp_vector(&builder, true, false).swap(bp);
}

void rmq_benchmark(size_t runs)
{
    srand(42); // make everything deterministic
    static const size_t sample_size = 10000000;

    std::cout << "SUCCINCT_EXCESS_RMQ" << std::endl;
    std::cout << "log_height" "\t" "excess_rmq_us" << std::endl;

    for (size_t ln = 10; ln <= 28; ln += 2) {
	size_t n = 1 << ln;
	double elapsed = 0;
	for (size_t run = 0; run < runs; ++run) {
            succinct::bp_vector bp;
	    build_random_binary_tree(bp, n);
	    elapsed += time_avg_rmq(bp, sample_size);
	}
	std::cout << ln << "\t" << elapsed / double(runs) << std::endl;
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

    rmq_benchmark(runs);
}
