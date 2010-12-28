#include "../../testing/testing.hpp"

#include "../../coding/reader.hpp"
#include "../../coding/writer.hpp"
#include "../../coding/streams_sink.hpp"

#include "../simple_tree.hpp"

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

UNIT_TEST(SimpleTree)
{
  typedef SimpleTree<int> mytree;
  mytree tree;

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
  TEST_EQUAL(tree[0].Value(), 1, ());
  TEST_EQUAL(tree[1].Value(), 2, ());
  TEST_EQUAL(tree[2].Value(), 3, ());
  TEST_EQUAL(tree[3].Value(), 4, ());
  TEST_EQUAL(tree[4].Value(), 5, ());
  TEST_EQUAL(tree[0][0].Value(), 10, ());
  TEST_EQUAL(tree[0][1].Value(), 20, ());
  TEST_EQUAL(tree[0][2].Value(), 30, ());

  Calculator<mytree> c1;
  tree.ForEachSibling(c1);
  TEST_EQUAL(c1.count, 5, ());

  Calculator<mytree> c2;
  tree.ForEachChildren(c2);
  TEST_EQUAL(c2.count, 8, ());

  tree.Clear();
  Calculator<mytree> c3;
  tree.ForEachChildren(c3);
  TEST_EQUAL(c3.count, 0, ("Tree should be empty"));
}
