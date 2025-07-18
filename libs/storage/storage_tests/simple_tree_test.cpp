#include "testing/testing.hpp"

#include "storage/country_tree.hpp"

UNIT_TEST(CountryTree_Smoke)
{
  using Tree = storage::CountryTree::Node;
  using Value = storage::Country;

  Tree tree(Value("0"), nullptr);

  tree.AddAtDepth(1, Value("4"));
  tree.AddAtDepth(1, Value("3"));
  tree.AddAtDepth(1, Value("5"));
  tree.AddAtDepth(1, Value("2"));
  tree.AddAtDepth(1, Value("1"));
  tree.AddAtDepth(2, Value("20"));
  tree.AddAtDepth(2, Value("10"));
  tree.AddAtDepth(2, Value("30"));

  // children test
  TEST_EQUAL(tree.Child(0).Value().Name(), "4", ());
  TEST_EQUAL(tree.Child(1).Value().Name(), "3", ());
  TEST_EQUAL(tree.Child(2).Value().Name(), "5", ());
  TEST_EQUAL(tree.Child(3).Value().Name(), "2", ());
  TEST_EQUAL(tree.Child(4).Value().Name(), "1", ());
  TEST_EQUAL(tree.Child(4).Child(0).Value().Name(), "20", ());
  TEST_EQUAL(tree.Child(4).Child(1).Value().Name(), "10", ());
  TEST_EQUAL(tree.Child(4).Child(2).Value().Name(), "30", ());

  // parent test
  TEST(!tree.HasParent(), ());
  TEST(!tree.Child(0).Parent().HasParent(), ());
  TEST_EQUAL(tree.Child(4).Child(0).Parent().Value().Name(), "1", ());
  TEST_EQUAL(tree.Child(4).Child(2).Parent().Value().Name(), "1", ());

  size_t count = 0;
  auto countCallback = [&count](Tree const &) { ++count; };
  tree.ForEachChild(countCallback);
  TEST_EQUAL(count, 5, ());

  count = 0;
  tree.ForEachDescendant(countCallback);
  TEST_EQUAL(count, 8, ());

  count = 0;
  tree.Child(4).Child(0).ForEachAncestorExceptForTheRoot(countCallback);
  TEST_EQUAL(count, 1, ());
}
