#include "base/SRC_FIRST.hpp"
#include "testing/testing.hpp"

#include "base/mru_cache.hpp"

UNIT_TEST(MRUCache_Test)
{
  my::MRUCache<int, int> m(4);

  CHECK(m.HasElem(1) == false, ());

  m.Add(1, 2, 1);

  CHECK(m.HasElem(1) == true, ());
  CHECK(m.Find(1, false) == 2, ());

  m.Add(2, 4, 2);

  CHECK(m.HasElem(2) == true, ());
  CHECK(m.Find(2, false) == 4, ());

  m.Touch(1);

  m.Add(3, 9, 2);

  CHECK(m.HasElem(3) == true, ());
  CHECK(m.Find(3, false) == 9, ());

  CHECK(m.HasElem(2) == false, ());
  CHECK(m.HasElem(1) == true, ());

  m.LockElem(1);
  m.Add(4, 16, 2);

  CHECK(m.HasElem(4) == true, ());
  CHECK(m.Find(4, false) == 16, ());

  CHECK(m.HasElem(3) == false, ());
  CHECK(m.HasElem(1) == true, ());

  m.UnlockElem(1);
  // 2 is still older than 4, so check this

  m.Add(5, 25, 2);
  CHECK(m.HasElem(5) == true, ());
  CHECK(m.HasElem(4) == true, ());
  CHECK(m.HasElem(1) == false, ());
}
