#pragma once

#include "generator/place_node.hpp"
#include "generator/regions/region.hpp"

#include <iostream>

namespace generator
{
namespace regions
{
using Node = PlaceNode<Region>;

size_t TreeSize(Node::Ptr node);

size_t MaxDepth(Node::Ptr node);

void DebugPrintTree(Node::Ptr tree, std::ostream & stream = std::cout);

// This function merges two trees if the roots have the same ids.
Node::Ptr MergeTree(Node::Ptr l, Node::Ptr r);

// This function corrects the tree. It traverses the whole node and unites children with
// the same ids.
void NormalizeTree(Node::Ptr tree);
}  // namespace regions
}  // namespace generator
