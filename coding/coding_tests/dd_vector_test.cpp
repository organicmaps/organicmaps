#include "../../testing/testing.hpp"
#include "../dd_vector.hpp"
#include "../endianness.hpp"
#include "../reader.hpp"
#include "../../base/start_mem_debug.hpp"

UNIT_TEST(DDVector)
{
  vector<unsigned char> data;
  // Push size. Big endian is used.
  data.push_back(1);
  data.push_back(2);
  data.push_back(3);
  typedef DDVector<unsigned char, MemReader> Vector;
  MemReader reader(reinterpret_cast<char const *>(&data[0]), data.size() * sizeof(data[0]));
  Vector v(reader, reader.Size()); // since sizeof(unsigned char) == 1
  TEST_EQUAL(3, v.size(), ());
  TEST_EQUAL(1, v[0], ());
  TEST_EQUAL(2, v[1], ());
  TEST_EQUAL(3, v[2], ());
  Vector::const_iterator it = v.begin();
  for (size_t i = 0; i < v.size(); ++i, ++it)
    TEST_EQUAL(v[i], *it, ());
}
