#include "testing/testing.hpp"

#include "storage/country_tree.hpp"

namespace
{
template <class TNode>
struct Calculator
{
  size_t count;
  Calculator() : count(0) {}
  void operator()(TNode const &)
  {
    ++count;
  }
};
} // namespace

UNIT_TEST(CountryTree_Smoke)
{
  typedef CountryTree<int, int>::Node TTree;
  TTree tree(0, nullptr);

  tree.AddAtDepth(1, 4);
  tree.AddAtDepth(1, 3);
  tree.AddAtDepth(1, 5);
  tree.AddAtDepth(1, 2);
  tree.AddAtDepth(1, 1);
  tree.AddAtDepth(2, 20);
  tree.AddAtDepth(2, 10);
  tree.AddAtDepth(2, 30);

  // children test
  TEST_EQUAL(tree.Child(0).Value(), 4, ());
  TEST_EQUAL(tree.Child(1).Value(), 3, ());
  TEST_EQUAL(tree.Child(2).Value(), 5, ());
  TEST_EQUAL(tree.Child(3).Value(), 2, ());
  TEST_EQUAL(tree.Child(4).Value(), 1, ());
  TEST_EQUAL(tree.Child(4).Child(0).Value(), 20, ());
  TEST_EQUAL(tree.Child(4).Child(1).Value(), 10, ());
  TEST_EQUAL(tree.Child(4).Child(2).Value(), 30, ());

  // parent test
  TEST(!tree.HasParent(), ());
  TEST(!tree.Child(0).Parent().HasParent(), ());
  TEST_EQUAL(tree.Child(4).Child(0).Parent().Value(), 1, ());
  TEST_EQUAL(tree.Child(4).Child(2).Parent().Value(), 1, ());

  Calculator<TTree> c1;
  tree.ForEachChild(c1);
  TEST_EQUAL(c1.count, 5, ());

  Calculator<TTree> c2;
  tree.ForEachDescendant(c2);
  TEST_EQUAL(c2.count, 8, ());

  Calculator<TTree> c3;
  tree.Child(4).Child(0).ForEachAncestorExceptForTheRoot(c3);
  TEST_EQUAL(c3.count, 1, ());
}
