#pragma once

#include "indexer/drawing_rules.hpp"

#include "geometry/rect2d.hpp"

#include "std/vector.hpp"

class FeatureType;
class ScreenBase;

namespace df
{
namespace watch
{

const int maxDepth = 20000;
const int minDepth = -20000;

class GlyphCache;

struct DrawRule
{
  drule::BaseRule const * m_rule;
  double m_depth;

  DrawRule() : m_rule(0), m_depth(0.0) {}
  DrawRule(drule::BaseRule const * p, double depth);

  uint32_t GetID(size_t threadSlot) const;
  void SetID(size_t threadSlot, uint32_t id) const;
};

struct FeatureStyler
{
  FeatureStyler() = default;
  FeatureStyler(FeatureType const & f,
                int const zoom,
                double const visualScale,
                GlyphCache * glyphCache,
                ScreenBase const * convertor,
                m2::RectD const * rect);

  typedef buffer_vector<DrawRule, 8> StylesContainerT;
  StylesContainerT m_rules;

  bool m_isCoastline;
  bool m_hasLineStyles;
  bool m_hasPointStyles;
  bool m_hasPathText;
  int m_geometryType;

  double m_visualScale;

  string m_primaryText;
  string m_secondaryText;
  string m_refText;

  typedef buffer_vector<double, 16> ClipIntervalsT;
  ClipIntervalsT m_intervals;

  typedef buffer_vector<double, 8> TextOffsetsT;
  TextOffsetsT m_offsets;

  uint8_t m_fontSize;

  GlyphCache * m_glyphCache;
  ScreenBase const * m_convertor;
  m2::RectD const * m_rect;

  double m_popRank;

  bool IsEmpty() const;

  string const GetPathName() const;

  bool FilterTextSize(drule::BaseRule const * pRule) const;

private:
  uint8_t GetTextFontSize(drule::BaseRule const * pRule) const;
  void LayoutTexts(double pathLength);
};

}
}
