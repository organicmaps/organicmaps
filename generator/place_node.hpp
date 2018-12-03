#pragma once

#include <memory>
#include <utility>
#include <vector>

namespace generator
{
// This is a tree node, where each node can have many children. Now it is used to build a hierarchy
// of places.
template <typename Data>
class PlaceNode
{
public:
  using Ptr = std::shared_ptr<PlaceNode>;
  using WeakPtr = std::weak_ptr<PlaceNode>;
  using PtrList = std::vector<Ptr>;

  explicit PlaceNode(Data && data) : m_data(std::move(data)) {}

  void AddChild(Ptr child) { m_children.push_back(child); }
  void SetParent(Ptr parent) { m_parent = parent; }
  void SetChildren(PtrList && children) { m_children = std::move(children); }
  void RemoveChildren() { m_children.clear(); }

  bool HasChildren() const { return !m_children.empty(); }
  bool HasParent() const { return m_parent.lock() != nullptr; }

  PtrList const & GetChildren() const { return m_children; }
  PtrList & GetChildren() { return m_children; }
  Ptr GetParent() const { return m_parent.lock(); }
  Data & GetData() { return m_data; }
  Data const & GetData() const { return m_data; }

private:
  Data m_data;
  PtrList m_children;
  WeakPtr m_parent;
};
}  // namespace generator
