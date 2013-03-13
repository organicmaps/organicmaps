#include "feature_styler.hpp"

#include "../indexer/drawing_rules.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/feature_visibility.hpp"
#ifdef OMIM_PRODUCTION
  #include "../indexer/drules_struct_lite.pb.h"
#else
  #include "../indexer/drules_struct.pb.h"
#endif

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
    vector<drule::Key> keys;
    string names;       // for debug use only, in release it's empty
    pair<int, bool> type = feature::GetDrawRule(f, zoom, keys, names);

    // don't try to do anything to invisible feature
    if (keys.empty())
      return;

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

    double area = 0.0;
    if (m_geometryType != GEOM_POINT)
    {
      m2::RectD const bbox = f.GetLimitRect(zoom);
      area = bbox.SizeX() * bbox.SizeY();
    }

    double priorityModifier;
    if (area != 0)
      priorityModifier = min(1., area*10000.); // making area larger so it's not lost on double conversions
    else
      priorityModifier = static_cast<double>(population) / 7E9;  // dividing by planet population to get priorityModifier < 1

    drule::MakeUnique(keys);

    int layer = f.GetLayer();
    bool isTransparent = false;
    if (layer == feature::LAYER_TRANSPARENT_TUNNEL)
      layer = 0;

    bool hasIcon = false;
    bool hasCaptionWithoutOffset = false;

    size_t const count = keys.size();
    m_rules.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
      double depth = keys[i].m_priority;

      if (layer != 0)
        depth = (layer * drule::layer_base_priority) + fmod(depth, drule::layer_base_priority);

      if (keys[i].m_type == drule::symbol)
        hasIcon = true;

      if ((keys[i].m_type == drule::caption)
       || (keys[i].m_type == drule::symbol)
       || (keys[i].m_type == drule::circle)
       || (keys[i].m_type == drule::pathtext)
       || (keys[i].m_type == drule::waymarker))
      {
        // show labels of larger objects first
        depth += priorityModifier;
      }
      else if (keys[i].m_type == drule::area)
      {
        // show smaller polygons on top
        depth -= priorityModifier;
      }

      if (!m_hasLineStyles && (keys[i].m_type == drule::line))
        m_hasLineStyles = true;

      m_rules[i] = di::DrawRule( drule::rules().Find(keys[i]), depth, isTransparent);

      if ((m_geometryType == GEOM_LINE) && !m_hasPathText && !m_primaryText.empty())
        if (m_rules[i].m_rule->GetCaption(0) != 0)
          m_hasPathText = true;

      if (keys[i].m_type == drule::caption)
        if (m_rules[i].m_rule->GetCaption(0) != 0)
          if (!m_rules[i].m_rule->GetCaption(0)->has_offset_y())
            hasCaptionWithoutOffset = true;
    }

    if (hasIcon && hasCaptionWithoutOffset)
      // we need to delete symbol style (single one due to MakeUnique call above)
      for (size_t i = 0; i < count; ++i)
      {
        if (keys[i].m_type == drule::symbol)
        {
          m_rules[i] = m_rules[m_rules.size() - 1];
          m_rules.pop_back();
          break;
        }
      }

    sort(m_rules.begin(), m_rules.end(), less_depth());
  }
}
