#include "testing/testing.hpp"

#include "storage/simple_tree.hpp"


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

}

UNIT_TEST(SimpleTree_Smoke)
{
  typedef SimpleTree<int> TreeT;
  TreeT tree;

  tree.Add(4);
  tree.Add(3);
  tree.Add(5);
  tree.Add(2);
  tree.Add(1);
  tree.AddAtDepth(1, 20);  // 1 is parent
  tree.AddAtDepth(1, 10);  // 1 is parent
  tree.AddAtDepth(1, 30);  // 1 is parent

  tree.Sort();
  // test sorting
  TEST_EQUAL(tree.Child(0).Value(), 1, ());
  TEST_EQUAL(tree.Child(1).Value(), 2, ());
  TEST_EQUAL(tree.Child(2).Value(), 3, ());
  TEST_EQUAL(tree.Child(3).Value(), 4, ());
  TEST_EQUAL(tree.Child(4).Value(), 5, ());
  TEST_EQUAL(tree.Child(0).Child(0).Value(), 10, ());
  TEST_EQUAL(tree.Child(0).Child(1).Value(), 20, ());
  TEST_EQUAL(tree.Child(0).Child(2).Value(), 30, ());

  Calculator<TreeT> c1;
  tree.ForEachChild(c1);
  TEST_EQUAL(c1.count, 5, ());

  Calculator<TreeT> c2;
  tree.ForEachDescendant(c2);
  TEST_EQUAL(c2.count, 8, ());

  tree.Clear();
  Calculator<TreeT> c3;
  tree.ForEachDescendant(c3);
  TEST_EQUAL(c3.count, 0, ("Tree should be empty"));
}
