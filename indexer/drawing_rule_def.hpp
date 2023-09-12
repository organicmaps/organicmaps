#pragma once

#include "base/buffer_vector.hpp"

namespace drule
{

// Priority range for area and line drules. Should be same as LAYER_PRIORITY_RANGE in kothic.
// Each layer = +/-1 value shifts the range by this number, so that e.g. priorities
// of the default layer=0 range [0;1000) don't intersect with layer=-1 range [-1000;0) and so on..
double constexpr kLayerPriorityRange = 1000;

// Should be same as OVERLAYS_MAX_PRIORITY in kothic.
// The overlays range is [-kOverlaysMaxPriority; kOverlaysMaxPriority), negative values are used
// for optional captions which are below all other overlays.
int32_t constexpr kOverlaysMaxPriority = 10000;

  class Key
  {
  public:
    int m_scale = -1;
    int m_type = -1;
    size_t m_index = std::numeric_limits<size_t>::max(); // an index to RulesHolder.m_dRules[]
    int m_priority = -1;
    bool m_hatching = false;

    Key() = default;
    Key(int s, int t, size_t i) : m_scale(s), m_type(t), m_index(i), m_priority(-1) {}

    bool operator==(Key const & r) const
    {
      return (m_index == r.m_index);
    }

    void SetPriority(int pr) { m_priority = pr; }
  };

  /// drawing type of rule - can be one of ...
  enum rule_type_t { line, area, symbol, caption, circle, pathtext, waymarker, shield, count_of_rules };

  /// text field type - can be one of ...
  enum text_type_t { text_type_name = 0, text_type_housename, text_type_housenumber };

  typedef buffer_vector<Key, 16> KeysT;
  void MakeUnique(KeysT & keys);
}
