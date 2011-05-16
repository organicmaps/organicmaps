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
    TEST_EQUAL ( dd1, v1[0].data(), () );

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

UNIT_TEST(BufferVectorZeroInitialized)
{
  for (size_t size = 0; size < 20; ++size)
  {
    buffer_vector<int, 5> v(size);
    for (size_t i = 0; i < size; ++i)
      TEST_EQUAL(v[i], 0, ());
  }
}

UNIT_TEST(BufferVectorResize)
{
  for (size_t size = 0; size < 20; ++size)
  {
    buffer_vector<int, 5> v;
    v.resize(size, 3);
    for (size_t i = 0; i < size; ++i)
      TEST_EQUAL(v[i], 3, ());
  }
}

UNIT_TEST(BufferVectorInsert)
{
  for (size_t initialLength = 0; initialLength < 20; ++initialLength)
  {
    for (size_t insertLength = 0; insertLength < 20; ++insertLength)
    {
      for (size_t insertPos = 0; insertPos <= initialLength; ++insertPos)
      {
        buffer_vector<char, 5> b;
        vector<char> v;
        for (size_t i = 0; i < initialLength; ++i)
        {
          b.push_back('A' + i);
          v.push_back('A' + i);
        }

        vector<int> dataToInsert(insertLength);
        for (size_t i = 0; i < insertLength; ++i)
          dataToInsert[i] = 'a' + i;

        b.insert(b.begin() + insertPos, dataToInsert.begin(), dataToInsert.end());
        v.insert(v.begin() + insertPos, dataToInsert.begin(), dataToInsert.end());

        vector<char> result(b.begin(), b.end());
        TEST_EQUAL(v, result, (initialLength, insertLength, insertPos));
      }
    }
  }
}

UNIT_TEST(BufferVectorPopBack)
{
  for (size_t len = 1; len < 6; ++len)
  {
    buffer_vector<int, 3> v;
    for (size_t i = 0; i < len; ++i)
      v.push_back(i);
    for (size_t i = len; i > 0; --i)
    {
      TEST(!v.empty(), (len, i));
      TEST_EQUAL(v.size(), i, ());
      TEST_EQUAL(v.front(), 0, ());
      TEST_EQUAL(v.back(), i-1, ());
      v.pop_back();
      TEST_EQUAL(v.size(), i-1, ());
    }
    TEST(v.empty(), ());
  }
}
