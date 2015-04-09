#include "testing/testing.hpp"
#include "base/cache.hpp"
#include "base/macros.hpp"
#include "base/stl_add.hpp"

UNIT_TEST(CacheSmoke)
{
  char const s     [] = "1123212434445";
  char const isNew [] = "1011???1??001";
  size_t const n = ARRAY_SIZE(s) - 1;
  for (int logCacheSize = 2; logCacheSize < 6; ++logCacheSize)
  {
    my::Cache<uint32_t, char> cache(logCacheSize);
    for (size_t i = 0; i < n; ++i)
    {
      bool found = false;
      char & c = cache.Find(s[i], found);
      if (isNew[i] != '?')
      {
        TEST_EQUAL(found, isNew[i] == '0', (i, s[i]));
      }
      if (found)
      {
        TEST_EQUAL(c, s[i], (i));
      }
      else
      {
        c = s[i];
      }
    }
  }
}

UNIT_TEST(CacheSmoke_0)
{
  my::Cache<uint32_t, char> cache(3);
  bool found = true;
  cache.Find(0, found);
  TEST(!found, ());
  vector<char> v;
  cache.ForEachValue(MakeBackInsertFunctor(v));
  TEST_EQUAL(v, vector<char>(8, 0), ());
}
