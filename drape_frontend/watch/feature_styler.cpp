#include "drape_frontend/watch/feature_styler.hpp"
#include "drape_frontend/watch/proto_to_styles.hpp"
#include "drape_frontend/watch/glyph_cache.hpp"
#include "drape_frontend/watch/geometry_processors.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"
#include "indexer/drules_include.hpp"

#include "geometry/screenbase.hpp"

#include "base/stl_add.hpp"
#include "base/logging.hpp"

#include "std/iterator_facade.hpp"

namespace
{

struct less_depth
{
  bool operator() (df::watch::DrawRule const & r1, df::watch::DrawRule const & r2) const
  {
    return (r1.m_depth < r2.m_depth);
  }
};

}

namespace df
{
namespace watch
{

DrawRule::DrawRule(drule::BaseRule const * p, double depth)
  : m_rule(p), m_depth(my::clamp(depth, minDepth, maxDepth))
{
}

uint32_t DrawRule::GetID(size_t threadSlot) const
{
  return m_rule->GetID(threadSlot);
}

void DrawRule::SetID(size_t threadSlot, uint32_t id) const
{
  m_rule->SetID(threadSlot, id);
}

FeatureStyler::FeatureStyler(FeatureType const & f,
                             int const zoom,
                             double const visualScale,
                             GlyphCache * glyphCache,
                             ScreenBase const * convertor,
                             m2::RectD const * rect)
  : m_hasPathText(false),
    m_visualScale(visualScale),
    m_glyphCache(glyphCache),
    m_convertor(convertor),
    m_rect(rect)
{
  drule::KeysT keys;
  pair<int, bool> type = feature::GetDrawRule(f, zoom, keys);

  // don't try to do anything to invisible feature
  if (keys.empty())
    return;

  m_hasLineStyles = false;
  m_hasPointStyles = false;

  m_geometryType = type.first;
  m_isCoastline = type.second;

  f.GetPreferredNames(m_primaryText, m_secondaryText);

  // Draw only one text for features on the World zoom level in user's native language.
  if (zoom <= scales::GetUpperWorldScale() && !m_secondaryText.empty())
  {
    m_primaryText.swap(m_secondaryText);
    m_secondaryText.clear();
  }

  // Low zoom heuristics - don't show superlong names on World map.
  //if (zoom <= 5)
  //{
  //  if (strings::MakeUniString(m_primaryText).size() > 50)
  //    m_primaryText.clear();
  //}

  string houseNumber;
  if (ftypes::IsBuildingChecker::Instance()(f))
  {
    houseNumber = f.GetHouseNumber();
    // Mark houses without names/numbers so user can select them by single tap.
    if (houseNumber.empty() && m_primaryText.empty())
      houseNumber = "·";
  }
  bool const hasName = !m_primaryText.empty() || !houseNumber.empty();

  m_refText = f.GetRoadNumber();

  double const population = static_cast<double>(f.GetPopulation());
  if (population == 1)
    m_popRank =  0.0;
  else
  {
    double const upperBound = 3.0E6;
    m_popRank = min(upperBound, population) / upperBound / 4;
  }

  double area = 0.0;
  if (m_geometryType != feature::GEOM_POINT)
  {
    m2::RectD const bbox = f.GetLimitRect(zoom);
    area = bbox.SizeX() * bbox.SizeY();
  }

  double priorityModifier;
  if (area != 0)
  {
    // making area larger so it's not lost on double conversions
    priorityModifier = min(1.0, area * 10000.0);
  }
  else
  {
    // dividing by planet population to get priorityModifier < 1
    priorityModifier = static_cast<double>(population) / 7E9;
  }

  drule::MakeUnique(keys);

  int layer = f.GetLayer();
  if (layer == feature::LAYER_TRANSPARENT_TUNNEL)
    layer = 0;

  bool hasIcon = false;
  bool hasCaptionWithoutOffset = false;

  m_fontSize = 0;

  size_t const count = keys.size();
  m_rules.resize(count);

  bool hasSecondaryText = false;

  for (size_t i = 0; i < count; ++i)
  {
    double depth = keys[i].m_priority;

    if (layer != 0 && depth < 19000)
    {
      if (keys[i].m_type == drule::line || keys[i].m_type == drule::waymarker)
        depth = (layer * drule::layer_base_priority) + fmod(depth, drule::layer_base_priority);
      else if (keys[i].m_type == drule::area)
      {
        // Use raw depth adding in area feature layers
        // (avoid overlap linear objects in case of "fmod").
        depth += layer * drule::layer_base_priority;
      }
    }

    if (keys[i].m_type == drule::symbol || keys[i].m_type == drule::circle)
      hasIcon = true;

    if ((keys[i].m_type == drule::caption && hasName)
     || (keys[i].m_type == drule::symbol)
     || (keys[i].m_type == drule::circle))
      m_hasPointStyles = true;

    if (keys[i].m_type == drule::caption
     || keys[i].m_type == drule::symbol
     || keys[i].m_type == drule::circle
     || keys[i].m_type == drule::pathtext)
    {
      // show labels of larger objects first
      depth += priorityModifier;
      // show labels of nodes first
      if (m_geometryType == feature::GEOM_POINT)
        ++depth;
    }
    else if (keys[i].m_type == drule::area)
    {
      // show smaller polygons on top
      depth -= priorityModifier;
    }

    if (!m_hasLineStyles && (keys[i].m_type == drule::line))
      m_hasLineStyles = true;

    m_rules[i] = DrawRule(drule::rules().Find(keys[i]), depth);

    if (m_rules[i].m_rule->GetCaption(1) != 0)
      hasSecondaryText = true;

    CaptionDefProto const * pCap0 = m_rules[i].m_rule->GetCaption(0);
    if (pCap0)
    {
      if (!m_hasPathText && hasName && (m_geometryType == feature::GEOM_LINE))
      {
        m_hasPathText = true;

        if (!FilterTextSize(m_rules[i].m_rule))
          m_fontSize = max(m_fontSize, GetTextFontSize(m_rules[i].m_rule));
      }

      if (keys[i].m_type == drule::caption)
        hasCaptionWithoutOffset = !(pCap0->has_offset_y() || pCap0->has_offset_x());
    }
  }

  // User's language name is better if we don't have secondary text draw rule.
  if (!hasSecondaryText && !m_secondaryText.empty() && (m_geometryType != feature::GEOM_LINE))
  {
    f.GetReadableName(m_primaryText);
    if (m_primaryText == m_secondaryText)
      m_secondaryText.clear();
  }

  // Get or concat house number if feature has one.
  if (!houseNumber.empty())
  {
    if (m_primaryText.empty() || houseNumber.find(m_primaryText) != string::npos)
      houseNumber.swap(m_primaryText);
    else
      m_primaryText = m_primaryText + " (" + houseNumber + ")";
  }

  // placing a text on the path
  if (m_hasPathText && (m_fontSize != 0))
  {
    typedef filter_screenpts_adapter<get_path_intervals> functor_t;

    functor_t::params p;

    p.m_convertor = m_convertor;
    p.m_rect = m_rect;
    p.m_intervals = &m_intervals;

    functor_t fun(p);
    f.ForEachPointRef(fun, zoom);

    LayoutTexts(fun.m_length);
  }

  if (hasIcon && hasCaptionWithoutOffset)
  {
    // we need to delete symbol style and circle style
    for (size_t i = 0; i < m_rules.size();)
    {
      if (keys[i].m_type == drule::symbol || keys[i].m_type == drule::circle)
      {
        m_rules[i] = m_rules[m_rules.size() - 1];
        m_rules.pop_back();
        keys[i] = keys[keys.size() - 1];
        keys.pop_back();
      }
      else
        ++i;
    }
  }

  sort(m_rules.begin(), m_rules.end(), less_depth());
}

typedef pair<double, double> RangeT;
template <class IterT> class RangeIterT :
    public iterator_facade<RangeIterT<IterT>, RangeT, forward_traversal_tag, RangeT>
{
  IterT m_iter;
public:
  RangeIterT(IterT iter) : m_iter(iter) {}

  RangeT dereference() const
  {
    IterT next = m_iter;
    ++next;
    return RangeT(*m_iter, *next);
  }
  bool equal(RangeIterT const & r) const { return (m_iter == r.m_iter); }
  void increment()
  {
    ++m_iter; ++m_iter;
  }
};

template <class ContT> class RangeInserter
{
  ContT & m_cont;
public:
  RangeInserter(ContT & cont) : m_cont(cont) {}

  RangeInserter & operator*() { return *this; }
  RangeInserter & operator++(int) { return *this; }
  RangeInserter & operator=(RangeT const & r)
  {
    m_cont.push_back(r.first);
    m_cont.push_back(r.second);
    return *this;
  }
};

void FeatureStyler::LayoutTexts(double pathLength)
{
  double const textLength = m_glyphCache->getTextLength(m_fontSize, GetPathName());
  /// @todo Choose best constant for minimal space.
  double const emptySize = max(200 * m_visualScale, textLength);
  // multiply on factor because tiles will be rendered in smaller scales
  double const minPeriodSize = 1.5 * (emptySize + textLength);

  size_t textCnt = 0;
  double firstTextOffset = 0;

  if (pathLength > textLength)
  {
    textCnt = ceil((pathLength - textLength) / minPeriodSize);
    firstTextOffset = 0.5 * (pathLength - (textCnt * textLength + (textCnt - 1) * emptySize));
  }

  if (textCnt != 0 && !m_intervals.empty())
  {
    buffer_vector<RangeT, 8> deadZones;

    for (size_t i = 0; i < textCnt; ++i)
    {
      double const deadZoneStart = firstTextOffset + minPeriodSize * i;
      double const deadZoneEnd = deadZoneStart + textLength;

      if (deadZoneStart > m_intervals.back())
        break;

      deadZones.push_back(make_pair(deadZoneStart, deadZoneEnd));
    }

    if (!deadZones.empty())
    {
      buffer_vector<double, 16> res;

      // accumulate text layout intervals with cliping intervals
      typedef RangeIterT<ClipIntervalsT::iterator> IterT;
      AccumulateIntervals1With2(IterT(m_intervals.begin()), IterT(m_intervals.end()),
                                deadZones.begin(), deadZones.end(),
                                RangeInserter<ClipIntervalsT>(res));

      m_intervals = res;
      ASSERT_EQUAL(m_intervals.size() % 2, 0, ());

      // get final text offsets (belongs to final clipping intervals)
      size_t i = 0;
      size_t j = 0;
      while (i != deadZones.size() && j != m_intervals.size())
      {
        ASSERT_LESS(deadZones[i].first, deadZones[i].second, ());
        ASSERT_LESS(m_intervals[j], m_intervals[j+1], ());

        if (deadZones[i].first < m_intervals[j])
        {
          ++i;
          continue;
        }
        if (m_intervals[j+1] <= deadZones[i].first)
        {
          j += 2;
          continue;
        }

        ASSERT_LESS_OR_EQUAL(m_intervals[j], deadZones[i].first, ());
        ASSERT_LESS_OR_EQUAL(deadZones[i].second, m_intervals[j+1], ());

        m_offsets.push_back(deadZones[i].first);
        ++i;
      }
    }
  }
}

string const FeatureStyler::GetPathName() const
{
  // Always concat names for linear features because we process only one draw rule now.
  if (m_secondaryText.empty())
    return m_primaryText;
  else
    return m_primaryText + "   " + m_secondaryText;
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
}
