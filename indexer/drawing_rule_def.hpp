#pragma once

#include "base/buffer_vector.hpp"


namespace drule
{
  class Key
  {
  public:
    int m_scale = -1;
    int m_type = -1;
    int m_index = -1;
    int m_priority = -1;
    bool m_hatching = false;

    Key() = default;
    Key(int s, int t, int i) : m_scale(s), m_type(t), m_index(i), m_priority(-1) {}

    bool operator==(Key const & r) const
    {
      return (m_scale == r.m_scale && m_type == r.m_type && m_index == r.m_index);
    }

    void SetPriority(int pr) { m_priority = pr; }
  };

  /// drawing type of rule - can be one of ...
  enum rule_type_t { line, area, symbol, caption, circle, pathtext, waymarker, shield, count_of_rules };

  /// text field type - can be one of ...
  enum text_type_t { text_type_name = 0, text_type_housename, text_type_housenumber };

  double const layer_base_priority = 2000;

  typedef buffer_vector<Key, 16> KeysT;
  void MakeUnique(KeysT & keys);
}
