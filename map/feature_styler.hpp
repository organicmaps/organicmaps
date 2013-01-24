#pragma once

#include "drawer.hpp"

#include "../std/vector.hpp"

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

    string m_primaryText;
    string m_secondaryText;
    string m_refText;

    double m_popRank;

    void GetStyles(FeatureType const & f, int const zoom);

    inline bool IsEmpty() const
    {
      return m_rules.empty();
    }
    inline size_t GetCount() const
    {
      return m_rules.size();
    }
  };
}
