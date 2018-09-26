#pragma once

#include "generator/regions/region.hpp"

#include <iostream>
#include <memory>
#include <vector>

namespace generator
{
namespace regions
{
struct Node
{
  using Ptr = std::shared_ptr<Node>;
  using WeakPtr = std::weak_ptr<Node>;
  using PtrList = std::vector<Ptr>;

  explicit Node(Region && region) : m_region(std::move(region)) {}

  void AddChild(Ptr child) { m_children.push_back(child); }
  PtrList const & GetChildren() const { return m_children; }
  PtrList & GetChildren() { return m_children; }
  void SetChildren(PtrList const children) { m_children = children; }
  void RemoveChildren() { m_children.clear(); }
  bool HasChildren() { return m_children.size(); }
  void SetParent(Ptr parent) { m_parent = parent; }
  Ptr GetParent() const { return m_parent.lock(); }
  Region & GetData() { return m_region; }

private:
  Region m_region;
  PtrList m_children;
  WeakPtr m_parent;
};

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
