#include "testing/testing.hpp"

#include "coding/dd_vector.hpp"
#include "coding/reader.hpp"

#include <cstdint>
#include <vector>

UNIT_TEST(DDVector_Smoke)
{
  std::vector<uint16_t> data;
  // Push size. Big endian is used.
  data.push_back(1);
  data.push_back(2);
  data.push_back(3);
  typedef DDVector<uint16_t, MemReader> Vector;
  MemReader reader(reinterpret_cast<char const *>(&data[0]), data.size() * sizeof(data[0]));
  Vector v(reader);
  TEST_EQUAL(3, v.size(), ());
  TEST_EQUAL(1, v[0], ());
  TEST_EQUAL(2, v[1], ());
  TEST_EQUAL(3, v[2], ());
  Vector::const_iterator it = v.begin();
  for (auto const value : v)
    TEST_EQUAL(value, *it++, ());
}

UNIT_TEST(DDVector_IncorrectSize)
{
  typedef DDVector<uint16_t, MemReader> Vector;
  char const data[] = "ab";
  MemReader reader(data, ARRAY_SIZE(data));

  bool exceptionCaught = false;
  try
  {
    Vector v(reader);
  }
  catch (Vector::OpenException & e)
  {
    exceptionCaught = true;
  }

  TEST(exceptionCaught, ());
}
