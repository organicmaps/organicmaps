#pragma once

#include "generator/place_node.hpp"
#include "generator/regions/level_region.hpp"

#include <iostream>

namespace generator
{
namespace regions
{
using Node = PlaceNode<LevelRegion>;
using NodePath = std::vector<Node::Ptr>;

NodePath MakeLevelPath(Node::Ptr const & node);

// The function has formally quadratic time complexity: depth * size of tree.
// In fact, tree depth is low value and thus the function time complexity is linear.
template <typename Fn>
void ForEachLevelPath(Node::Ptr const & tree, Fn && fn)
{
  if (!tree)
    return;

  if (tree->GetData().GetLevel() != PlaceLevel::Unknown)
    fn(MakeLevelPath(tree));

  for (auto const & subtree : tree->GetChildren())
    ForEachLevelPath(subtree, fn);
}

size_t TreeSize(Node::Ptr node);

size_t MaxDepth(Node::Ptr node);

void DebugPrintTree(Node::Ptr const & tree, std::ostream & stream = std::cout);

// This function merges two trees if the roots have the same ids.
Node::Ptr MergeTree(Node::Ptr l, Node::Ptr r);

// This function corrects the tree. It traverses the whole node and unites children with
// the same ids.
void NormalizeTree(Node::Ptr tree);
}  // namespace regions
}  // namespace generator
