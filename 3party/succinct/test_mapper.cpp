#define BOOST_TEST_MODULE mapper
#include "test_common.hpp"

#include <boost/filesystem.hpp>

#include "mapper.hpp"

BOOST_AUTO_TEST_CASE(basic_map)
{
    succinct::mapper::mappable_vector<int> vec;
    BOOST_REQUIRE_EQUAL(vec.size(), 0U);

    int nums[] = {1, 2, 3, 4};
    vec.assign(nums);

    BOOST_REQUIRE_EQUAL(4U, vec.size());
    BOOST_REQUIRE_EQUAL(1, vec[0]);
    BOOST_REQUIRE_EQUAL(4, vec[3]);

    succinct::mapper::freeze(vec, "temp.bin");

    {
        succinct::mapper::mappable_vector<int> mapped_vec;
        boost::iostreams::mapped_file_source m("temp.bin");
        succinct::mapper::map(mapped_vec, m);
        BOOST_REQUIRE_EQUAL(vec.size(), mapped_vec.size());
        BOOST_REQUIRE(std::equal(vec.begin(), vec.end(), mapped_vec.begin()));
    }

    boost::filesystem::remove("temp.bin");
}

class complex_struct {
public:
    complex_struct()
        : m_a(0)
    {}

    void init() {
        m_a = 42;
        uint32_t b[] = {1, 2};
        m_b.assign(b);
    }

    template <typename Visitor>
    void map(Visitor& visit) {
        visit
            (m_a, "m_a")
            (m_b, "m_b")
            ;
    }

    uint64_t m_a;
    succinct::mapper::mappable_vector<uint32_t> m_b;
};

BOOST_AUTO_TEST_CASE(complex_struct_map)
{
    complex_struct s;
    s.init();
    succinct::mapper::freeze(s, "temp.bin");

    BOOST_REQUIRE_EQUAL(24, succinct::mapper::size_of(s));

    complex_struct mapped_s;
    BOOST_REQUIRE_EQUAL(0, mapped_s.m_a);
    BOOST_REQUIRE_EQUAL(0U, mapped_s.m_b.size());

    {
        boost::iostreams::mapped_file_source m("temp.bin");
        succinct::mapper::map(mapped_s, m);
        BOOST_REQUIRE_EQUAL(s.m_a, mapped_s.m_a);
        BOOST_REQUIRE_EQUAL(s.m_b.size(), mapped_s.m_b.size());
    }

    boost::filesystem::remove("temp.bin");
}
