#pragma once

#include "storage/country.hpp"
#include "storage/storage_defines.hpp"

#include "base/buffer_vector.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
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

    Node(Country && value, Node * parent) : m_value(std::move(value)), m_parent(parent) {}

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
    Node * AddAtDepth(size_t level, Country && value);

    bool HasParent() const { return m_parent != nullptr; }

    bool IsRoot() const { return !HasParent(); }

    Node const & Parent() const;

    Node const & Child(size_t index) const;

    size_t ChildrenCount() const { return m_children.size(); }

    /// \brief Calls |f| for each first generation descendant of the node.
    void ForEachChild(NodeCallback const & f);

    void ForEachChild(NodeCallback const & f) const;

    /// \brief Calls |f| for all nodes (add descendant) in the tree.
    void ForEachDescendant(NodeCallback const & f);

    void ForEachDescendant(NodeCallback const & f) const;

    void ForEachInSubtree(NodeCallback const & f);

    void ForEachInSubtree(NodeCallback const & f) const;

    void ForEachAncestorExceptForTheRoot(NodeCallback const & f);

    void ForEachAncestorExceptForTheRoot(NodeCallback const & f) const;

  private:
    Node * Add(Country && value);

    Country m_value;

    /// \brief m_children contains all first generation descendants of the node.
    /// Note. Once created the order of elements of |m_children| should not be changed.
    /// See implementation of AddAtDepth and Add methods for details.
    std::vector<std::unique_ptr<Node>> m_children;
    Node * m_parent;
  };

  bool IsEmpty() const { return m_countryTree == nullptr; }

  Node const & GetRoot() const;

  Node & GetRoot();

  Country & AddAtDepth(size_t level, Country && value);

  /// Deletes all children and makes tree empty
  void Clear();

  using NodesBufferT = buffer_vector<Node const *, 4>;
  /// \brief Checks all nodes in tree to find an equal one. If there're several equal nodes
  /// returns the first found.
  /// \returns a poiter item in the tree if found and nullptr otherwise.
  void Find(CountryId const & key, NodesBufferT & found) const;

  Node const * FindFirst(CountryId const & key) const;

  /// \brief Find only leaves.
  /// \note It's a termprary fucntion for compatablity with old countries.txt.
  /// When new countries.txt with unique ids will be added FindLeaf will be removed
  /// and Find will be used intead.
  /// @TODO(bykoianko) Remove this method on countries.txt update.
  Node const * FindFirstLeaf(CountryId const & key) const;

private:
  std::unique_ptr<Node> m_countryTree;
  std::multimap<CountryId, Node *> m_countryTreeMap;
};

/// @return version of country file or -1 if error was encountered
int64_t LoadCountriesFromBuffer(std::string const & buffer, CountryTree & countries, Affiliations & affiliations,
                                CountryNameSynonyms & countryNameSynonyms, MwmTopCityGeoIds & mwmTopCityGeoIds,
                                MwmTopCountryGeoIds & mwmTopCountryGeoIds);
int64_t LoadCountriesFromFile(std::string const & path, CountryTree & countries, Affiliations & affiliations,
                              CountryNameSynonyms & countryNameSynonyms, MwmTopCityGeoIds & mwmTopCityGeoIds,
                              MwmTopCountryGeoIds & mwmTopCountryGeoIds);

void LoadCountryFile2CountryInfo(std::string const & jsonBuffer, std::map<std::string, CountryInfo> & id2info);
}  // namespace storage
