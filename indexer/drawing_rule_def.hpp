#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"


namespace drule
{
  class Key
  {
  public:
    int m_scale;
    int m_type;
    int m_index;
    int m_priority;

    Key() : m_scale(-1), m_type(-1), m_index(-1), m_priority(-1) {}
    Key(int s, int t, int i) : m_scale(s), m_type(t), m_index(i), m_priority(-1) {}

    bool operator==(Key const & r) const
    {
      return (m_scale == r.m_scale && m_type == r.m_type && m_index == r.m_index);
    }

    string toString() const;
    void fromString(string const & s);

    void SetPriority(int pr) { m_priority = pr; }
  };

  /// drawing type of rule - can be one of ...
  enum rule_type_t { line, area, symbol, caption, circle, pathtext, waymarker, count_of_rules };

  /// geo type of rule - can be one combined of ...
  enum rule_geo_t { node = 1, way = 2 };

  int const layer_base_priority = 2000;

  void MakeUnique(vector<Key> & keys);
  void SortByScaleTypeDepth(vector<Key> & keys);
}
