#include "feature_styler.hpp"
#include "geometry_processors.hpp"
#include "proto_to_styles.hpp"

#include "../indexer/drawing_rules.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/feature_visibility.hpp"

#ifdef OMIM_PRODUCTION
  #include "../indexer/drules_struct_lite.pb.h"
#else
  #include "../indexer/drules_struct.pb.h"
#endif

#include "../geometry/screenbase.hpp"

#include "../graphics/glyph_cache.hpp"

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

namespace di
{
  uint32_t DrawRule::GetID(size_t threadSlot) const
  {
    return (m_transparent ? m_rule->GetID2(threadSlot) : m_rule->GetID(threadSlot));
  }

  void DrawRule::SetID(size_t threadSlot, uint32_t id) const
  {
    m_transparent ? m_rule->SetID2(threadSlot, id) : m_rule->SetID(threadSlot, id);
  }

  FeatureStyler::FeatureStyler(FeatureType const & f,
                               int const zoom,
                               double const visualScale,
                               graphics::GlyphCache * glyphCache,
                               ScreenBase const * convertor,
                               m2::RectD const * rect)
    : m_hasPathText(false),
      m_visualScale(visualScale),
      m_glyphCache(glyphCache),
      m_convertor(convertor),
      m_rect(rect)
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

    if (m_geometryType != feature::GEOM_POINT)
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

    bool pathWasClipped = false;
    m_fontSize = 0;

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

      if ((m_geometryType == feature::GEOM_LINE) && !m_hasPathText && !m_primaryText.empty())
        if (m_rules[i].m_rule->GetCaption(0) != 0)
        {
          m_hasPathText = true;

          /// calculating clip intervals only once

          if (!pathWasClipped)
          {
            typedef gp::filter_screenpts_adapter<gp::get_path_intervals> functor_t;

            functor_t::params p;

            p.m_convertor = m_convertor;
            p.m_rect = m_rect;
            p.m_intervals = &m_intervals;

            functor_t fun(p);

            f.ForEachPointRef(fun, zoom);

            m_pathLength = fun.m_length;
            pathWasClipped = true;
          }

          if (!FilterTextSize(m_rules[i].m_rule))
            m_fontSize = max(m_fontSize, GetTextFontSize(m_rules[i].m_rule));
        }

      if (keys[i].m_type == drule::caption)
        if (m_rules[i].m_rule->GetCaption(0) != 0)
          if (!m_rules[i].m_rule->GetCaption(0)->has_offset_y())
            hasCaptionWithoutOffset = true;
    }

    /// placing a text on the path
    if (m_hasPathText && (m_fontSize != 0))
      LayoutTexts();

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

  void FeatureStyler::LayoutTexts()
  {
    m_textLength = m_glyphCache->getTextLength(m_fontSize, GetPathName());
    double emptySize = max(200 * m_visualScale, m_textLength);
    double minPeriodSize = emptySize + m_textLength;

    size_t textCnt = 0;
    double firstTextOffset = 0;

    if (m_pathLength > m_textLength)
    {
      textCnt = ceil((m_pathLength - m_textLength) / minPeriodSize);
      firstTextOffset = 0.5 * (m_pathLength - (textCnt * m_textLength + (textCnt - 1) * emptySize));
    }

    if ((textCnt != 0) && (!m_intervals.empty()))
    {
      buffer_vector<double, 16> deadZones;

      for (unsigned i = 0; i < textCnt; ++i)
      {
        double deadZoneStart = firstTextOffset + (m_textLength + minPeriodSize) * i;
        double deadZoneEnd = deadZoneStart + m_textLength;

        if (deadZoneStart > m_intervals.back())
          break;

        deadZones.push_back(deadZoneStart);
        deadZones.push_back(deadZoneEnd);
        m_offsets.push_back(deadZoneStart);
      }

      if (!deadZones.empty())
      {
        buffer_vector<double, 16> res(deadZones.size() + m_intervals.size());

        int k = my::MergeSorted(&m_intervals[0], m_intervals.size(),
                                &deadZones[0], deadZones.size(),
                                &res[0], res.size());

        res.resize(k);

        m_intervals = res;
      }
    }
  }

  string const FeatureStyler::GetPathName() const
  {
    if (m_secondaryText.empty())
      return m_primaryText;
    else
      return m_primaryText + "      " + m_secondaryText;
  }

  bool FeatureStyler::IsEmpty() const
  {
    return m_rules.empty();
  }

  uint8_t FeatureStyler::GetTextFontSize(drule::BaseRule const * pRule) const
  {
    return pRule->GetCaption(0)->height() * m_visualScale;
  }

  bool FeatureStyler::FilterTextSize(drule::BaseRule const * pRule) const
  {
    if (pRule->GetCaption(0))
      return (GetFontSize(pRule->GetCaption(0)) < 3);
    else
    {
      // this rule is not a caption at all
      return true;
    }
  }
}
