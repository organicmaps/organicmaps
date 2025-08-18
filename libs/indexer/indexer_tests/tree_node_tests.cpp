#include "testing/testing.hpp"

#include "indexer/complex/tree_node.hpp"

#include "base/control_flow.hpp"

#include <iterator>
#include <list>
#include <string>
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
  tree_node::PreOrderVisit(tree, [&](auto const & node) { res.emplace_back(node->GetData()); });
  std::vector<int> const expected = {1, 21, 31, 32, 33, 34, 22, 35, 36};
  TEST_EQUAL(res, expected, ());

  auto countVisitedNode = 0;
  tree_node::PreOrderVisit(tree, [&](auto const & node)
  {
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

UNIT_TEST(TreeNode_GetRoot)
{
  auto const tree = MakeTree();
  auto const node22 = tree_node::FindIf(tree, [](auto const & d) { return d == 22; });
  auto const root = tree_node::GetRoot(node22);
  TEST(tree, ());
  TEST_EQUAL(tree->GetData(), root->GetData(), ());
}

UNIT_TEST(TreeNode_GetPath)
{
  auto const tree = MakeTree();
  auto const node33 = tree_node::FindIf(tree, [](auto const & d) { return d == 33; });
  auto const path = tree_node::GetPathToRoot(node33);
  tree_node::types::Ptrs<int> expected = {tree_node::FindIf(tree, [](auto const & d) { return d == 33; }),
                                          tree_node::FindIf(tree, [](auto const & d) { return d == 21; }),
                                          tree_node::FindIf(tree, [](auto const & d) { return d == 1; })};
  TEST_EQUAL(path, expected, ());
}

UNIT_TEST(TreeNode_IsEqual)
{
  auto tree1 = tree_node::MakeTreeNode(1);
  auto node11 = tree_node::MakeTreeNode(21);
  tree_node::Link(tree_node::MakeTreeNode(31), node11);
  tree_node::Link(tree_node::MakeTreeNode(32), node11);
  tree_node::Link(tree_node::MakeTreeNode(33), node11);
  tree_node::Link(node11, tree1);

  TEST(tree_node::IsEqual(tree1, tree1), ());
  TEST(!tree_node::IsEqual(tree1, node11), ());
}

UNIT_TEST(TreeNode_TransformToTree)
{
  auto const tree = MakeTree();
  auto const newTree = tree_node::TransformToTree(tree, [](auto const & data) { return std::to_string(data); });

  auto expected = tree_node::MakeTreeNode<std::string>("1");
  auto node11 = tree_node::MakeTreeNode<std::string>("21");
  tree_node::Link(tree_node::MakeTreeNode<std::string>("31"), node11);
  tree_node::Link(tree_node::MakeTreeNode<std::string>("32"), node11);
  tree_node::Link(tree_node::MakeTreeNode<std::string>("33"), node11);
  tree_node::Link(node11, expected);
  tree_node::Link(tree_node::MakeTreeNode<std::string>("34"), expected);

  auto tree2 = tree_node::MakeTreeNode<std::string>("22");
  tree_node::Link(tree_node::MakeTreeNode<std::string>("35"), tree2);
  tree_node::Link(tree_node::MakeTreeNode<std::string>("36"), tree2);
  tree_node::Link(tree2, expected);

  TEST(tree_node::IsEqual(newTree, expected), ());
}

UNIT_TEST(TreeNode_CountIf)
{
  auto const tree = MakeTree();
  auto const count = tree_node::CountIf(tree, [](auto const & data) { return data >= 30; });
  TEST_EQUAL(count, 6, ());
}

UNIT_TEST(TreeNode_DebugPrint)
{
  auto const tree = MakeTree();
  LOG(LINFO, (tree_node::DebugPrint(tree)));
}

UNIT_TEST(TreeNode_Forest)
{
  tree_node::Forest<int> forest;
  std::set<tree_node::types::Ptr<int>> s = {MakeTree(), tree_node::MakeTreeNode(10)};
  for (auto const & tree : s)
    forest.Append(tree);

  TEST_EQUAL(forest.Size(), 2, ());
  forest.ForEachTree([&](auto const & tree)
  {
    auto const count = s.erase(tree);
    TEST_EQUAL(count, 1, ());
  });
  TEST(s.empty(), ());

  auto const copy = forest;
  TEST_EQUAL(copy, forest, ());

  tree_node::Forest<int> empty;
  TEST_NOT_EQUAL(empty, forest, ());
}

UNIT_TEST(TreeNode_ForestFindIf)
{
  tree_node::Forest<int> forest;
  forest.Append(MakeTree());
  forest.Append(tree_node::MakeTreeNode(100));
  auto const node33 = tree_node::FindIf(forest, [](auto const & d) { return d == 33; });
  TEST(node33, ());
  TEST_EQUAL(node33->GetData(), 33, ());

  auto const node100 = tree_node::FindIf(forest, [](auto const & d) { return d == 100; });
  TEST(node100, ());
  TEST_EQUAL(node100->GetData(), 100, ());
}

UNIT_TEST(TreeNode_ForestDebugPrint)
{
  tree_node::Forest<int> forest;
  forest.Append(MakeTree());
  forest.Append(tree_node::MakeTreeNode(100));
  forest.Append(tree_node::MakeTreeNode(200));
  LOG(LINFO, (tree_node::DebugPrint(forest)));
}
}  // namespace
