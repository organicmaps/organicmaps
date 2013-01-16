#pragma once

#include "../std/vector.hpp"
#include "drawer.hpp"

class FeatureType;

namespace feature
{
  class StylesContainer
  {

  public:
    StylesContainer();
    ~StylesContainer();

    typedef buffer_vector<di::DrawRule, 16> StylesContainerT;
    StylesContainerT m_rules;
    bool m_isCoastline;
    bool m_hasLineStyles;
    bool m_hasPathText;
    int m_geometryType;
    size_t m_count;

    string m_primaryText;
    string m_secondaryText;
    string m_refText;

    double m_popRank;

    void GetStyles(FeatureType const & f, int const zoom);
    inline bool IsEmpty() const
    {
      return m_rules.empty();
    }
  };
}
