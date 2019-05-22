#include "testing/testing.hpp"

#include "storage/country_tree.hpp"

namespace
{
template <class Node>
struct Calculator
{
  size_t count;
  Calculator() : count(0) {}
  void operator()(Node const &) { ++count; }
};
} // namespace

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

  Calculator<Tree> c1;
  tree.ForEachChild(c1);
  TEST_EQUAL(c1.count, 5, ());

  Calculator<Tree> c2;
  tree.ForEachDescendant(c2);
  TEST_EQUAL(c2.count, 8, ());

  Calculator<Tree> c3;
  tree.Child(4).Child(0).ForEachAncestorExceptForTheRoot(c3);
  TEST_EQUAL(c3.count, 1, ());
}
