#include "../SRC_FIRST.hpp"

#include "../../testing/testing.hpp"

#include "../buffer_vector.hpp"


namespace 
{
  template <class TCont>
  void CheckVector(TCont & cont, size_t count)
  {
    TEST_EQUAL ( cont.size(), count, () );
    for (size_t i = 0; i < count; ++i)
      TEST_EQUAL ( cont[i], i, () );
  }
}

UNIT_TEST(BufferVectorBounds)
{
  buffer_vector<int, 2> v;

  for (size_t i = 0; i < 5; ++i)
  {
    v.push_back(i);
    CheckVector(v, i+1);
  }

  v.resize(2);
  CheckVector(v, 2);

  v.resize(3);
  v[2] = 2;
  CheckVector(v, 3);

  v.resize(4);
  v[3] = 3;
  CheckVector(v, 4);

  v.resize(1);
  CheckVector(v, 1);

  v.resize(0);
  CheckVector(v, 0);
}

UNIT_TEST(BufferVectorSwap)
{
  typedef buffer_vector<int, 2> value_t;
  buffer_vector<value_t, 2> v1, v2;

  for (size_t i = 0; i < 5; ++i)
  {
    v1.push_back(value_t());
    v2.push_back(value_t());
  }

  // check swap of vectors
  value_t const * d1 = v1.data();
  value_t const * d2 = v2.data();

  swap(v1, v2);
  TEST_EQUAL ( d1, v2.data(), () );
  TEST_EQUAL ( d2, v1.data(), () );

  // check swap in resized data
  {
    v1[0].push_back(666);

    // inner static buffer doesn't swap
    int const * dd1 = v1[0].data();

    v1.resize(1);
    TEST_EQUAL ( v1[0].size(), 1, () );
    TEST_EQUAL ( v1[0][0], 666, () );
    TEST_NOT_EQUAL ( dd1, v1[0].data(), () );

    v1.resize(7);
    TEST_EQUAL ( v1[0].size(), 1, () );
    TEST_EQUAL ( v1[0][0], 666, () );
    TEST_NOT_EQUAL ( dd1, v1[0].data(), () );
  }

  {
    for (size_t i = 0; i < 5; ++i)
      v2[0].push_back(i);

    // inner dynamic buffer should be swapped
    int const * dd2 = v2[0].data();

    v2.resize(1);
    TEST_EQUAL ( v2[0].size(), 5, () );
    TEST_EQUAL ( dd2, v2[0].data(), () );

    v1.resize(7);
    TEST_EQUAL ( v2[0].size(), 5, () );
    TEST_EQUAL ( dd2, v2[0].data(), () );
  }
}
