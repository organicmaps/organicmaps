#pragma once

#include "storage/country.hpp"
#include "storage/storage_defines.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace storage
{
/// \brief This class is developed for using in Storage. It's a implementation of a tree with
/// ability
/// of access to its nodes in expected constant time with the help of hash table.
/// It should be filled with AddAtDepth method.
/// This class is used in Storage and filled based on countries.txt (countries_migrate.txt).
/// While filling CountryTree nodes in countries.txt should be visited in DFS order.
class CountryTree
{
public:
  /// This class is developed for using in CountryTree. It's a implementation of a tree.
  /// It should be filled with AddAtDepth method.
  /// This class is used in filled based on countries.txt (countries_migrate.txt).
  /// While filling Node nodes in countries.txt should be visited in DFS order.
  class Node
  {
  public:
    using NodeCallback = std::function<void(Node const &)>;

    Node(Country const & value, Node * parent) : m_value(value), m_parent(parent) {}

    Country const & Value() const { return m_value; }
    Country & Value() { return m_value; }

    /// \brief Adds a node to a tree with root == |this|.
    /// \param level is depth where a node with |value| will be add. |level| == 0 means |this|.
    /// When the method is called the node |this| exists. So it's wrong call it with |level| == 0.
    /// If |level| == 1 the node with |value| will be added as a child of this.
    /// If |level| == 2 the node with |value| will be added as a child of the last added child of the |this|.
    /// And so on.
    /// \param value is a value of node which will be added.
    /// \note This method does not let to add a node to an arbitrary place in the tree.
    /// It's posible to add children only from "right side".
    Node * AddAtDepth(size_t level, Country const & value)
    {
      Node * node = this;
      while (--level > 0 && !node->m_children.empty())
        node = node->m_children.back().get();
      ASSERT_EQUAL(level, 0, ());
      return node->Add(value);
    }

    bool HasParent() const { return m_parent != nullptr; }

    bool IsRoot() const { return !HasParent(); }

    Node const & Parent() const
    {
      CHECK(HasParent(), ());
      return *m_parent;
    }

    Node const & Child(size_t index) const
    {
      ASSERT_LESS(index, m_children.size(), ());
      return *m_children[index];
    }

    size_t ChildrenCount() const { return m_children.size(); }

    /// \brief Calls |f| for each first generation descendant of the node.
    void ForEachChild(NodeCallback const & f)
    {
      for (auto & child : m_children)
        f(*child);
    }

    void ForEachChild(NodeCallback const & f) const
    {
      for (auto const & child : m_children)
        f(*child);
    }

    /// \brief Calls |f| for all nodes (add descendant) in the tree.
    void ForEachDescendant(NodeCallback const & f)
    {
      for (auto & child : m_children)
      {
        f(*child);
        child->ForEachDescendant(f);
      }
    }

    void ForEachDescendant(NodeCallback const & f) const
    {
      for (auto const & child : m_children)
      {
        f(*child);
        child->ForEachDescendant(f);
      }
    }

    void ForEachInSubtree(NodeCallback const & f)
    {
      f(*this);
      for (auto & child : m_children)
        child->ForEachInSubtree(f);
    }

    void ForEachInSubtree(NodeCallback const & f) const
    {
      f(*this);
      for (auto const & child : m_children)
        child->ForEachInSubtree(f);
    }

    void ForEachAncestorExceptForTheRoot(NodeCallback const & f)
    {
      if (m_parent == nullptr || m_parent->m_parent == nullptr)
        return;
      f(*m_parent);
      m_parent->ForEachAncestorExceptForTheRoot(f);
    }

    void ForEachAncestorExceptForTheRoot(NodeCallback const & f) const
    {
      if (m_parent == nullptr || m_parent->m_parent == nullptr)
        return;
      f(*m_parent);
      m_parent->ForEachAncestorExceptForTheRoot(f);
    }

  private:
    Node * Add(Country const & value)
    {
      m_children.emplace_back(std::make_unique<Node>(value, this));
      return m_children.back().get();
    }

    Country m_value;

    /// \brief m_children contains all first generation descendants of the node.
    /// Note. Once created the order of elements of |m_children| should not be changed.
    /// See implementation of AddAtDepth and Add methods for details.
    std::vector<std::unique_ptr<Node>> m_children;
    Node * m_parent;
  };

  bool IsEmpty() const { return m_countryTree == nullptr; }

  Node const & GetRoot() const
  {
    CHECK(m_countryTree, ());
    return *m_countryTree;
  }

  Node & GetRoot()
  {
    CHECK(m_countryTree, ());
    return *m_countryTree;
  }

  Country & AddAtDepth(size_t level, Country const & value)
  {
    Node * added = nullptr;
    if (level == 0)
    {
      ASSERT(IsEmpty(), ());
      m_countryTree = std::make_unique<Node>(value, nullptr);  // Creating the root node.
      added = m_countryTree.get();
    }
    else
    {
      added = m_countryTree->AddAtDepth(level, value);
    }

    ASSERT(added, ());
    m_countryTreeHashTable.insert(make_pair(value.Name(), added));
    return added->Value();
  }

  /// Deletes all children and makes tree empty
  void Clear()
  {
    m_countryTree.reset();
    m_countryTreeHashTable.clear();
  }

  /// \brief Checks all nodes in tree to find an equal one. If there're several equal nodes
  /// returns the first found.
  /// \returns a poiter item in the tree if found and nullptr otherwise.
  void Find(CountryId const & key, std::vector<Node const *> & found) const
  {
    found.clear();
    if (IsEmpty())
      return;

    if (key == m_countryTree->Value().Name())
      found.push_back(m_countryTree.get());

    auto const range = m_countryTreeHashTable.equal_range(key);
    if (range.first == range.second)
      return;

    std::for_each(range.first, range.second,
                  [&found](typename CountryTreeHashTable::value_type const & node) {
                    found.push_back(node.second);
                  });
  }

  Node const * const FindFirst(CountryId const & key) const
  {
    if (IsEmpty())
      return nullptr;

    std::vector<Node const *> found;
    Find(key, found);
    if (found.empty())
      return nullptr;
    return found[0];
  }

  /// \brief Find only leaves.
  /// \note It's a termprary fucntion for compatablity with old countries.txt.
  /// When new countries.txt with unique ids will be added FindLeaf will be removed
  /// and Find will be used intead.
  /// @TODO(bykoianko) Remove this method on countries.txt update.
  Node const * const FindFirstLeaf(CountryId const & key) const
  {
    if (IsEmpty())
      return nullptr;

    std::vector<Node const *> found;
    Find(key, found);

    for (auto node : found)
    {
      if (node->ChildrenCount() == 0)
        return node;
    }
    return nullptr;
  }

private:
  using CountryTreeHashTable = std::multimap<CountryId, Node *>;

  std::unique_ptr<Node> m_countryTree;
  CountryTreeHashTable m_countryTreeHashTable;
};

/// @return version of country file or -1 if error was encountered
int64_t LoadCountriesFromBuffer(std::string const & buffer, CountryTree & countries,
                                Affiliations & affiliations, OldMwmMapping * mapping = nullptr);
int64_t LoadCountriesFromFile(std::string const & path, CountryTree & countries,
                              Affiliations & affiliations, OldMwmMapping * mapping = nullptr);

void LoadCountryFile2CountryInfo(std::string const & jsonBuffer,
                                 std::map<std::string, CountryInfo> & id2info, bool & isSingleMwm);
}  // namespace storage
