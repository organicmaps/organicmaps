#include "testing/testing.hpp"

#include "base/linked_map.hpp"

#include <map>
#include <string>

UNIT_TEST(LinkedMap_Smoke)
{
  base::LinkedMap<int, std::string, std::map> container;

  TEST(container.IsEmpty(), ());

  TEST(container.Emplace(1, "hello"), ());
  TEST(container.Emplace(2, "world"), ());
  TEST(container.Emplace(3, "!"), ());

  TEST(!container.IsEmpty(), ());
  TEST_EQUAL(container.Size(), 3, ());

  TEST(!container.Emplace(2, "again"), ());

  TEST_EQUAL(container.Size(), 3, ());

  TEST(container.Contains(2), ());
  auto const getResult = container.Get(2);
  TEST_EQUAL(getResult, "world", ());

  TEST_EQUAL(container.Front(), "hello", ());
  container.PopFront();
  TEST_EQUAL(container.Front(), "world", ());

  TEST_EQUAL(container.Size(), 2, ());

  TEST(!container.Contains(10), ());
  TEST(!container.Erase(10), ());
  TEST_EQUAL(container.Size(), 2, ());

  TEST(container.Contains(3), ());
  TEST(container.Erase(3), ());
  TEST_EQUAL(container.Size(), 1, ());
  TEST_EQUAL(container.Front(), "world", ());

  decltype(container) otherContainer;
  otherContainer.Swap(container);

  TEST(container.IsEmpty(), ());
  TEST_EQUAL(container.Size(), 0, ());
  TEST(!otherContainer.IsEmpty(), ());
  TEST_EQUAL(otherContainer.Size(), 1, ());
  TEST_EQUAL(otherContainer.Front(), "world", ());
}
