#include "testing/testing.hpp"

#include "indexer/complex/tree_node.hpp"

#include "base/control_flow.hpp"

#include <iterator>
#include <list>
#include <vector>

namespace
{
decltype(auto) MakeTree()
{
  auto tree1 = tree_node::MakeTreeNode(1);

  auto node11 = tree_node::MakeTreeNode(21);
  tree_node::Link(tree_node::MakeTreeNode(31), node11);
  tree_node::Link(tree_node::MakeTreeNode(32), node11);
  tree_node::Link(tree_node::MakeTreeNode(33), node11);
  tree_node::Link(node11, tree1);
  tree_node::Link(tree_node::MakeTreeNode(34), tree1);

  auto tree2 = tree_node::MakeTreeNode(22);
  tree_node::Link(tree_node::MakeTreeNode(35), tree2);
  tree_node::Link(tree_node::MakeTreeNode(36), tree2);
  tree_node::Link(tree2, tree1);
  return tree1;
}

UNIT_TEST(TreeNode_PreOrderVisit)
{
  auto const tree = MakeTree();
  std::vector<int> res;
  tree_node::PreOrderVisit(tree, [&](auto const & node) {
    res.emplace_back(node->GetData());
  });
  std::vector<int> const expected = {1, 21, 31, 32, 33, 34, 22, 35, 36};
  TEST_EQUAL(res, expected, ());

  auto countVisitedNode = 0;
  tree_node::PreOrderVisit(tree, [&](auto const & node) {
    ++countVisitedNode;
    return node->GetData() == 32 ? base::ControlFlow::Break : base::ControlFlow::Continue;
  });
  TEST_EQUAL(countVisitedNode, 4, ());
}

UNIT_TEST(TreeNode_Size)
{
  auto const tree = MakeTree();
  auto const size = tree_node::Size(tree);
  TEST_EQUAL(size, 9, ());
}

UNIT_TEST(TreeNode_FindIf)
{
  auto const tree = MakeTree();
  auto node = tree_node::FindIf(tree, [](auto const & d) { return d == 1; });
  TEST(node, ());

  node = tree_node::FindIf(tree, [](auto const & d) { return d == 22; });
  TEST(node, ());

  node = tree_node::FindIf(tree, [](auto const & d) { return d == 36; });
  TEST(node, ());

  node = tree_node::FindIf(tree, [](auto const & d) { return d == 100; });
  TEST(!node, ());
}

UNIT_TEST(TreeNode_GetDepth)
{
  auto const tree = MakeTree();
  auto depth = tree_node::GetDepth(tree_node::FindIf(tree, [](auto const & d) { return d == 1; }));
  TEST_EQUAL(depth, 1, ());

  depth = tree_node::GetDepth(tree_node::FindIf(tree, [](auto const & d) { return d == 22; }));
  TEST_EQUAL(depth, 2, ());

  depth = tree_node::GetDepth(tree_node::FindIf(tree, [](auto const & d) { return d == 36; }));
  TEST_EQUAL(depth, 3, ());
}
}  // namespace
