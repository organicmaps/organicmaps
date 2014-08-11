#pragma once

#include "test_common.hpp"

template <class Vector>
inline void test_equal_bits(std::vector<bool> const& v, Vector const& bitmap, const char* test_name)
{
    BOOST_REQUIRE_EQUAL(v.size(), bitmap.size());
    for (size_t i = 0; i < v.size(); ++i) {
        MY_REQUIRE_EQUAL((bool)v[i], bitmap[i],
                         "operator[] (" << test_name << "): i=" << i);
    }
}

template <class Vector>
void test_rank_select0(std::vector<bool> const& v, Vector const& bitmap, const char* test_name)
{
    uint64_t cur_rank0 = 0;
    uint64_t last_zero = uint64_t(-1);

    for (size_t i = 0; i < v.size(); ++i) {
        MY_REQUIRE_EQUAL(cur_rank0, bitmap.rank0(i),
                         "rank0 (" << test_name << "): cur_rank0 = " << cur_rank0 << ", i = " << i << ", v[i] = " << v[i]);
        if (!v[i]) {
            last_zero = i;
            MY_REQUIRE_EQUAL(last_zero, bitmap.select0(cur_rank0),
                             "select0 (" << test_name << "): cur_rank0 = " << cur_rank0 << ", i = " << i << ", v[i] = " << v[i] << ", last_zero = " << last_zero);
            ++cur_rank0;
        }
        if (last_zero != uint64_t(-1)) {
            MY_REQUIRE_EQUAL(last_zero, bitmap.predecessor0(i),
                             "predecessor0 (" << test_name << "): last_zero = " << last_zero <<", i = " << i << ",v[i] = " << v[i]);
        }
    }

    last_zero = uint64_t(-1);
    for (size_t i = v.size() - 1; i + 1 > 0; --i) {
        if (!v[i]) {
            last_zero = i;
        }

        if (last_zero != uint64_t(-1)) {
            MY_REQUIRE_EQUAL(last_zero, bitmap.successor0(i),
                             "successor0 (" << test_name << "): last_zero = " << last_zero <<", i = " << i << ",v[i] = " << v[i]);
        }
    }
}

template <class Vector>
void test_rank_select1(std::vector<bool> const& v, Vector const& bitmap, const char* test_name)
{
    uint64_t cur_rank = 0;
    uint64_t last_one = uint64_t(-1);

    for (size_t i = 0; i < v.size(); ++i) {
        MY_REQUIRE_EQUAL(cur_rank, bitmap.rank(i),
                         "rank (" << test_name << "): cur_rank = " << cur_rank << ", i = " << i << ", v[i] = " << v[i]);

        if (v[i]) {
            last_one = i;
            MY_REQUIRE_EQUAL(last_one, bitmap.select(cur_rank),
                             "select (" << test_name << "): cur_rank = " << cur_rank << ", i = " << i << ", v[i] = " << v[i] << ", last_one = " << last_one);
            ++cur_rank;
        }

        if (last_one != uint64_t(-1)) {
            MY_REQUIRE_EQUAL(last_one, bitmap.predecessor1(i),
                             "predecessor1 (" << test_name << "): last_one = " << last_one <<", i = " << i << ",v[i] = " << v[i]);
        }
    }

    last_one = uint64_t(-1);
    for (size_t i = v.size() - 1; i + 1 > 0; --i) {
        if (v[i]) {
            last_one = i;
        }

        if (last_one != uint64_t(-1)) {
            MY_REQUIRE_EQUAL(last_one, bitmap.successor1(i),
                             "successor1 (" << test_name << "): last_one = " << last_one <<", i = " << i << ",v[i] = " << v[i]);
        }
    }
}

template <class Vector>
void test_rank_select(std::vector<bool> const& v, Vector const& bitmap, const char* test_name)
{
    test_rank_select0(v, bitmap, test_name);
    test_rank_select1(v, bitmap, test_name);
}

template <class Vector>
void test_delta(Vector const& bitmap, const char* test_name)
{
    for (size_t i = 0; i < bitmap.num_ones(); ++i) {
        if (i) {
            MY_REQUIRE_EQUAL(bitmap.select(i) - bitmap.select(i - 1),
                             bitmap.delta(i),
                             "delta (" << test_name << "), i = " << i);
        } else {
            MY_REQUIRE_EQUAL(bitmap.select(i),
                             bitmap.delta(i),
                             "delta (" << test_name << "), i = " << i);
        }
    }
}

template <class Vector>
void test_select_enumeration(std::vector<bool> const& v, Vector const& bitmap, const char* test_name)
{
    // XXX test other starting points
    typename Vector::select_enumerator it(bitmap, 0);

    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i]) {
            uint64_t res = it.next();
            MY_REQUIRE_EQUAL(i,
                             res,
                             "select_iterator next (" << test_name << "), i = " << i
                             << ", n = " << bitmap.size() << ", m = " << bitmap.num_ones());
        }
    }
}
