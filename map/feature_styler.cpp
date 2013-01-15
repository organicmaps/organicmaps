#include "feature_styler.hpp"


namespace
{
  struct less_depth
  {
    bool operator() (di::DrawRule const & r1, di::DrawRule const & r2) const
    {
      return (r1.m_depth < r2.m_depth);
    }
  };
}



namespace feature
{
  StylesContainer::StylesContainer() {}

  StylesContainer::~StylesContainer()
  {
    //for_each(m_rules.begin(), m_rules.end(), DeleteFunctor());
  }

  void StylesContainer::GetStyles(FeatureType const & f, int const zoom)
  {
    // do special stuff
    vector<drule::Key> keys;
    string names;       // for debug use only, in release it's empty
    pair<int, bool> type = feature::GetDrawRule(f, zoom, keys, names);

    m_hasLineStyles = false;

    m_geometryType = type.first;
    m_isCoastline = type.second;

    f.GetPreferredDrawableNames(m_primaryText, m_secondaryText);
    m_refText = f.GetRoadNumber();

    double const population = static_cast<double>(f.GetPopulation());
    if (population == 1)
      m_popRank =  0.0;
    else
    {
      double const upperBound = 3.0E6;
      m_popRank = min(upperBound, population) / upperBound / 4;
    }

    // low zoom heuristics
    if (zoom <= 5)
    {
      // hide superlong names on low zoom
      if (m_primaryText.size() > 50)
        m_primaryText.clear();
    }

    m_priorityModifier = 0;

    //m_secondaryText = strings::to_string(population);

    if (population != 0){
      // dividing by planet population to get m_priorityModifier < 1
      m_priorityModifier = static_cast<double>(population) / 7E9;
    }

    drule::MakeUnique(keys);
    size_t const count = keys.size();

    int layer = f.GetLayer();
    bool isTransparent = false;
    if (layer == feature::LAYER_TRANSPARENT_TUNNEL)
    {
      layer = 0;
      isTransparent = true;
    }

    m_rules.resize(count);

    for (size_t i = 0; i < count; ++i)
    {
      double depth = keys[i].m_priority;
      if (layer != 0)
        depth = (layer * drule::layer_base_priority) + fmod(depth, drule::layer_base_priority);
      depth += m_priorityModifier;

      m_rules[i] = ( di::DrawRule( drule::rules().Find(keys[i]), depth, isTransparent) );

      if (!m_hasLineStyles && (keys[i].m_type == drule::line))
        m_hasLineStyles = true;

      if (m_hasLineStyles && !m_hasPathText && !m_primaryText.empty())
        if (m_rules[i].m_rule->GetCaption(0) != 0)
          m_hasPathText = true;

    }

    sort(m_rules.begin(), m_rules.end(), less_depth());
    m_count = m_rules.size();
  }
}
