#pragma once

#include "../std/vector.hpp"
#include "../indexer/drawing_rules.hpp"
#include "drawer.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/feature_visibility.hpp"

namespace feature
{
  class StylesContainer
  {

    //buffer_vector<di::DrawRule, 16> rules;


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
    double m_priorityModifier;

    void GetStyles(FeatureType const & f, int const zoom);
    bool empty()
    {
      return m_rules.empty();
    }

//    void Assign(buffer_vector<di::DrawRule, 16> & rules);
  };
}
