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

size_t TreeSize(Node::Ptr const & node);

size_t MaxDepth(Node::Ptr const & node);

void DebugPrintTree(Node::Ptr const & tree, std::ostream & stream = std::cout);
}  // namespace regions
}  // namespace generator
