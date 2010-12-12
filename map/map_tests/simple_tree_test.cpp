#include "../../testing/testing.hpp"

#include "../../platform/platform.hpp"

#include "../../std/fstream.hpp"

#include "../simple_tree.hpp"
#include "../countries.hpp"

using namespace mapinfo;

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
  typedef SimpleTree<CountryTreeNode> mytree;
  mytree tree;
  std::ifstream file(GetPlatform().WritablePathForFile("countries.txt").c_str());
  TEST( LoadCountries(tree, file), ("countries.txt is absent or corrupted"));

  tree.Sort();

  Calculator<mytree> c1;
  tree.ForEachSibling(c1);
  TEST_GREATER_OR_EQUAL(c1.count, 6, ("At least 6 continents should be present"));

  // find europe
  size_t i = 0;
  for (; i < tree.SiblingsCount(); ++i)
    if (tree[i].Value().Name() == "Europe")
      break;
  TEST_LESS(i, tree.SiblingsCount(), ("Europe is not present?"));

  Calculator<mytree> c2;
  tree[i].ForEachSibling(c2);
  TEST_GREATER_OR_EQUAL(c2.count, 20, ("At least 20 countries should be present in Europe"));

  Calculator<mytree> c3;
  tree[i].ForEachChildren(c3);
  TEST_GREATER(c3.count, c2.count, ("Europe should contain some regions"));
}
