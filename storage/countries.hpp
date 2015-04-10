#pragma once
#include "base/base.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

template <class T> class SimpleTree;

namespace mapinfo
{
  /// used in SimpleTree when loading countries.txt
  class CountryTreeNode
  {
    /// group, country or region
    string m_name;
    /// cell ids for the country
    vector<uint16_t> m_ids;

  public:
    CountryTreeNode(string const & name = string()) : m_name(name) {}

    bool operator<(CountryTreeNode const & other) const
    {
      return m_name < other.m_name;
    }

    string const & Name() const { return m_name; }

    void AddCellId(string const & cellId);
  };

  /// loads tree from given stream
  bool LoadCountries(SimpleTree<CountryTreeNode> & tree, std::istream & stream);
}
