#include "drawer.hpp"
#include "feature_info.hpp"

#include "indexer/drules_include.hpp"
#include "indexer/feature_decl.hpp"

#include "base/buffer_vector.hpp"
#include "base/macros.hpp"

Drawer::Params::Params()
  : m_visualScale(1)
{
}

Drawer::Drawer(Params const & params)
  : m_visualScale(params.m_visualScale)
{
}

double Drawer::VisualScale() const
{
  return m_visualScale;
}

void Drawer::SetScale(int level)
{
  m_level = level;
}

void Drawer::DrawFeatureStart(FeatureID const & id)
{
  m_currentFeatureID = id;
}

void Drawer::DrawFeatureEnd(FeatureID const & id)
{
  ASSERT_EQUAL(m_currentFeatureID, id, ());
  UNUSED_VALUE(id);
  m_currentFeatureID = FeatureID();
}

FeatureID const & Drawer::GetCurrentFeatureID() const
{
  ASSERT(m_currentFeatureID.IsValid(), ());
  return m_currentFeatureID;
}

void Drawer::GenerateRoadNumbers(di::PathInfo const & path, di::FeatureStyler const & fs, TRoadNumberCallbackFn const & fn)
{
  int const textHeight = static_cast<int>(11 * VisualScale());
  m2::PointD pt;
  double const length = path.GetFullLength();
  if (length >= (fs.m_refText.size() + 2) * textHeight)
  {
    size_t const count = size_t(length / 1000.0) + 2;

    for (size_t j = 1; j < count; ++j)
    {
      if (path.GetSmPoint(double(j) / double(count), pt))
      {
        graphics::FontDesc fontDesc(textHeight, graphics::Color(150, 75, 0, 255),   // brown
                                    true, graphics::Color::White());

        fn(pt, fontDesc, fs.m_refText);
      }
    }
  }
}

void Drawer::Draw(di::FeatureInfo const & fi)
{
  DrawFeatureStart(fi.m_id);

  buffer_vector<di::DrawRule, 8> const & rules = fi.m_styler.m_rules;
  buffer_vector<di::DrawRule, 8> pathRules;

  bool const isPath = !fi.m_pathes.empty();
  bool const isArea = !fi.m_areas.empty();
  bool isCircleAndSymbol = false;
  drule::BaseRule const * pCircleRule = NULL;
  double circleDepth = graphics::minDepth;
  drule::BaseRule const * pSymbolRule = NULL;
  double symbolDepth = graphics::minDepth;

  // separating path rules from other
  for (size_t i = 0; i < rules.size(); ++i)
  {
    drule::BaseRule const * pRule = rules[i].m_rule;

    bool const isSymbol = pRule->GetSymbol() != 0;
    bool const isCaption = pRule->GetCaption(0) != 0;
    bool const isCircle = pRule->GetCircle() != 0;

    if (pSymbolRule == NULL && isSymbol)
    {
      pSymbolRule = pRule;
      symbolDepth = rules[i].m_depth;
    }

    if (pCircleRule == NULL && isCircle)
    {
      pCircleRule = pRule;
      circleDepth = rules[i].m_depth;
    }

    if (!isCaption && isPath && !isSymbol && (pRule->GetLine() != 0))
      pathRules.push_back(rules[i]);
  }

  isCircleAndSymbol = (pSymbolRule != NULL) && (pCircleRule != NULL);

  if (!pathRules.empty())
  {
    for (list<di::PathInfo>::const_iterator i = fi.m_pathes.begin(); i != fi.m_pathes.end(); ++i)
      DrawPath(*i, pathRules.data(), pathRules.size());
  }

  for (size_t i = 0; i < rules.size(); ++i)
  {
    drule::BaseRule const * pRule = rules[i].m_rule;
    double const depth = rules[i].m_depth;

    bool const isCaption = pRule->GetCaption(0) != 0;
    bool const isSymbol = pRule->GetSymbol() != 0;
    bool const isCircle = pRule->GetCircle() != 0;

    if (!isCaption)
    {
      // draw area
      if (isArea)
      {
        bool const isFill = pRule->GetArea() != 0;

        for (list<di::AreaInfo>::const_iterator i = fi.m_areas.begin(); i != fi.m_areas.end(); ++i)
        {
          if (isFill)
            DrawArea(*i, di::DrawRule(pRule, depth));
          else if (isCircleAndSymbol && isCircle)
          {
            DrawCircledSymbol(i->GetCenter(),
                              graphics::EPosCenter,
                              di::DrawRule(pSymbolRule, symbolDepth),
                              di::DrawRule(pCircleRule, circleDepth));
          }
          else if (isSymbol)
            DrawSymbol(i->GetCenter(), graphics::EPosCenter, di::DrawRule(pRule, depth));
          else if (isCircle)
            DrawCircle(i->GetCenter(), graphics::EPosCenter, di::DrawRule(pRule, depth));
        }
      }

      // draw point symbol
      if (!isPath && !isArea && ((pRule->GetType() & drule::node) != 0))
      {
        if (isCircleAndSymbol)
        {
          DrawCircledSymbol(fi.m_point,
                            graphics::EPosCenter,
                            di::DrawRule(pSymbolRule, symbolDepth),
                            di::DrawRule(pCircleRule, circleDepth));
        }
        else if (isSymbol)
          DrawSymbol(fi.m_point, graphics::EPosCenter, di::DrawRule(pRule, depth));
        else if (isCircle)
          DrawCircle(fi.m_point, graphics::EPosCenter, di::DrawRule(pRule, depth));
      }
    }
    else
    {
      if (!fi.m_styler.m_primaryText.empty() && (pRule->GetCaption(0) != 0))
      {
        bool isN = ((pRule->GetType() & drule::way) != 0);

        graphics::EPosition textPosition = graphics::EPosCenter;
        if (pRule->GetCaption(0)->has_offset_y())
        {
          if (pRule->GetCaption(0)->offset_y() > 0)
            textPosition = graphics::EPosUnder;
          else
            textPosition = graphics::EPosAbove;
        }
        if (pRule->GetCaption(0)->has_offset_x())
        {
          if (pRule->GetCaption(0)->offset_x() > 0)
            textPosition = graphics::EPosRight;
          else
            textPosition = graphics::EPosLeft;
        }

        // draw area text
        if (isArea/* && isN*/)
        {
          for (list<di::AreaInfo>::const_iterator i = fi.m_areas.begin(); i != fi.m_areas.end(); ++i)
            DrawText(i->GetCenter(), textPosition, fi.m_styler, di::DrawRule(pRule, depth));
        }

        // draw way name
        if (isPath && !isArea && isN && !fi.m_styler.FilterTextSize(pRule))
        {
          for (list<di::PathInfo>::const_iterator i = fi.m_pathes.begin(); i != fi.m_pathes.end(); ++i)
            DrawPathText(*i, fi.m_styler, di::DrawRule(pRule, depth));
        }

        // draw point text
        isN = ((pRule->GetType() & drule::node) != 0);
        if (!isPath && !isArea && isN)
          DrawText(fi.m_point, textPosition, fi.m_styler, di::DrawRule(pRule, depth));
      }
    }
  }

  // draw road numbers
  if (isPath && !fi.m_styler.m_refText.empty() && m_level >= 12)
    for (auto n : fi.m_pathes)
      DrawPathNumber(n, fi.m_styler);

  DrawFeatureEnd(fi.m_id);
}
