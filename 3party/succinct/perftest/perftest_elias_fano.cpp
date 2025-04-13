#include <iostream>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "util.hpp"

#include "elias_fano.hpp"
#include "mapper.hpp"

#include "perftest_common.hpp"

struct monotone_generator
{
    monotone_generator(uint64_t m, uint8_t bits, unsigned int seed)
	: m_gen(seed)
        , m_bits(bits)
    {
	m_stack.push_back(state_t(0, m, 0));
    }

    uint64_t next()
    {
	uint64_t cur_word, cur_m;
	uint8_t cur_depth;

	assert(m_stack.size());
	boost::tie(cur_word, cur_m, cur_depth) = m_stack.back();
	m_stack.pop_back();

	while (cur_depth < m_bits) {
	    boost::random::uniform_int_distribution<uint64_t> dist(0, cur_m);
	    uint64_t left_m = dist(m_gen);
	    uint64_t right_m = cur_m - left_m;

	    // push left and right children, if present
	    if (right_m > 0) {
		m_stack.push_back(state_t(cur_word | (uint64_t(1) << (m_bits - cur_depth - 1)),
					  right_m, cur_depth + 1));
	    }
	    if (left_m > 0) {
		m_stack.push_back(state_t(cur_word, left_m, cur_depth + 1));

	    }

	    // pop next child in visit
	    boost::tie(cur_word, cur_m, cur_depth) = m_stack.back();
	    m_stack.pop_back();
	}

	if (cur_m > 1) {
	    // push back the current leaf, with cur_m decreased by one
	    m_stack.push_back(state_t(cur_word, cur_m - 1, cur_depth));
	}

	return cur_word;
    }

    bool done() const
    {
	return m_stack.empty();
    }

private:
    typedef boost::tuple<uint64_t /* cur_word */,
			 uint64_t /* cur_m */,
			 uint64_t /* cur_depth */> state_t;
    std::vector<state_t> m_stack;
    boost::random::mt19937 m_gen;
    uint8_t m_bits;
};

void ef_enumeration_benchmark(uint64_t m, uint8_t bits)
{
    succinct::elias_fano::elias_fano_builder bvb(uint64_t(1) << bits, m);
    monotone_generator mgen(m, bits, 37);
    for (size_t i = 0; i < m; ++i) {
	bvb.push_back(mgen.next());
    }
    assert(mgen.done());

    succinct::elias_fano ef(&bvb);
    succinct::mapper::size_tree_of(ef)->dump();


    double elapsed;
    uint64_t foo = 0;
    SUCCINCT_TIMEIT(elapsed) {
	succinct::elias_fano::select_enumerator it(ef, 0);
	for (size_t i = 0; i < m; ++i) {
	    foo ^= it.next();
	}
    }
    volatile uint64_t vfoo = foo;
    (void)vfoo; // silence warning

    std::cerr << "Elapsed: " << elapsed / 1000 << " msec\n"
	      << double(m) / elapsed << " Mcodes/s" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "Invalid arguments" << std::endl;
        std::terminate();
    }
    size_t m = boost::lexical_cast<uint64_t>(argv[1]);
    uint8_t bits = uint8_t(boost::lexical_cast<int>(argv[2]));

    ef_enumeration_benchmark(m, bits);
}
