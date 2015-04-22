/*

Copyright (c) 2014, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "../../data_structures/binary_heap.hpp"
#include "../../typedefs.h"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <random>

BOOST_AUTO_TEST_SUITE(binary_heap)

struct TestData
{
    unsigned value;
};

typedef NodeID TestNodeID;
typedef int TestKey;
typedef int TestWeight;
typedef boost::mpl::list<ArrayStorage<TestNodeID, TestKey>,
                         MapStorage<TestNodeID, TestKey>,
                         UnorderedMapStorage<TestNodeID, TestKey>> storage_types;

template <unsigned NUM_ELEM> struct RandomDataFixture
{
    RandomDataFixture()
    {
        for (unsigned i = 0; i < NUM_ELEM; i++)
        {
            data.push_back(TestData{i * 3});
            weights.push_back((i + 1) * 100);
            ids.push_back(i);
            order.push_back(i);
        }

        // Choosen by a fair W20 dice roll
        std::mt19937 g(15);

        std::shuffle(order.begin(), order.end(), g);
    }

    std::vector<TestData> data;
    std::vector<TestWeight> weights;
    std::vector<TestNodeID> ids;
    std::vector<unsigned> order;
};

constexpr unsigned NUM_NODES = 100;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(insert_test, T, storage_types, RandomDataFixture<NUM_NODES>)
{
    BinaryHeap<TestNodeID, TestKey, TestWeight, TestData, T> heap(NUM_NODES);

    TestWeight min_weight = std::numeric_limits<TestWeight>::max();
    TestNodeID min_id;

    for (unsigned idx : order)
    {
        BOOST_CHECK(!heap.WasInserted(ids[idx]));

        heap.Insert(ids[idx], weights[idx], data[idx]);

        BOOST_CHECK(heap.WasInserted(ids[idx]));

        if (weights[idx] < min_weight)
        {
            min_weight = weights[idx];
            min_id = ids[idx];
        }
        BOOST_CHECK_EQUAL(min_id, heap.Min());
    }

    for (auto id : ids)
    {
        const auto &d = heap.GetData(id);
        BOOST_CHECK_EQUAL(d.value, data[id].value);

        const auto &w = heap.GetKey(id);
        BOOST_CHECK_EQUAL(w, weights[id]);
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(delete_min_test, T, storage_types, RandomDataFixture<NUM_NODES>)
{
    BinaryHeap<TestNodeID, TestKey, TestWeight, TestData, T> heap(NUM_NODES);

    for (unsigned idx : order)
    {
        heap.Insert(ids[idx], weights[idx], data[idx]);
    }

    for (auto id : ids)
    {
        BOOST_CHECK(!heap.WasRemoved(id));

        BOOST_CHECK_EQUAL(heap.Min(), id);
        BOOST_CHECK_EQUAL(id, heap.DeleteMin());
        if (id + 1 < NUM_NODES)
            BOOST_CHECK_EQUAL(heap.Min(), id + 1);

        BOOST_CHECK(heap.WasRemoved(id));
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(delete_all_test, T, storage_types, RandomDataFixture<NUM_NODES>)
{
    BinaryHeap<TestNodeID, TestKey, TestWeight, TestData, T> heap(NUM_NODES);

    for (unsigned idx : order)
    {
        heap.Insert(ids[idx], weights[idx], data[idx]);
    }

    heap.DeleteAll();

    BOOST_CHECK(heap.Empty());
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(decrease_key_test, T, storage_types, RandomDataFixture<10>)
{
    BinaryHeap<TestNodeID, TestKey, TestWeight, TestData, T> heap(10);

    for (unsigned idx : order)
    {
        heap.Insert(ids[idx], weights[idx], data[idx]);
    }

    std::vector<TestNodeID> rids(ids);
    std::reverse(rids.begin(), rids.end());

    for (auto id : rids)
    {
        TestNodeID min_id = heap.Min();
        TestWeight min_weight = heap.GetKey(min_id);

        // decrease weight until we reach min weight
        while (weights[id] > min_weight)
        {
            heap.DecreaseKey(id, weights[id]);
            BOOST_CHECK_EQUAL(heap.Min(), min_id);
            weights[id]--;
        }

        // make weight smaller than min
        weights[id] -= 2;
        heap.DecreaseKey(id, weights[id]);
        BOOST_CHECK_EQUAL(heap.Min(), id);
    }
}

BOOST_AUTO_TEST_SUITE_END()
