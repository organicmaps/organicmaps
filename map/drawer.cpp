#include "map/drawer.hpp"
#include "map/proto_to_styles.hpp"
#include "map/feature_styler.hpp"

#include "std/bind.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/scales.hpp"

#include "graphics/defines.hpp"
#include "graphics/screen.hpp"
#include "graphics/resource_manager.hpp"
#include "graphics/straight_text_element.hpp"

#include "indexer/drules_include.hpp"
#include "indexer/feature.hpp"

#include "geometry/screenbase.hpp"

#include "base/logging.hpp"
#include "base/buffer_vector.hpp"

Drawer::Params::Params()
  : m_visualScale(1)
{
}

Drawer::Drawer(Params const & params)
  : m_visualScale(params.m_visualScale), m_pScreen(new graphics::Screen(params))
{
  for (unsigned i = 0; i < m_pScreen->pipelinesCount(); ++i)
    m_pScreen->addClearPageFn(i, bind(&Drawer::ClearResourceCache, ThreadSlot(), i), 0);
}

namespace
{
  struct DoMakeInvalidRule
  {
    size_t m_threadSlot;
    uint32_t m_pipelineIDMask;

    DoMakeInvalidRule(size_t threadSlot, uint8_t pipelineID)
      : m_threadSlot(threadSlot), m_pipelineIDMask(pipelineID << 24)
    {}

    void operator() (int, int, int, drule::BaseRule * p)
    {
      if ((p->GetID(m_threadSlot) & 0xFF000000) == m_pipelineIDMask)
        p->MakeEmptyID(m_threadSlot);
    }
  };
}

void Drawer::ClearResourceCache(size_t threadSlot, uint8_t pipelineID)
{
  drule::rules().ForEachRule(DoMakeInvalidRule(threadSlot, pipelineID));
}

void Drawer::beginFrame()
{
  m_pScreen->beginFrame();
}

void Drawer::endFrame()
{
  m_pScreen->endFrame();
}

void Drawer::clear(graphics::Color const & c, bool clearRT, float depth, bool clearDepth)
{
  m_pScreen->clear(c, clearRT, depth, clearDepth);
}

void Drawer::onSize(int w, int h)
{
  m_pScreen->onSize(w, h);
}

void Drawer::drawSymbol(m2::PointD const & pt,
                        string const & symbolName,
                        graphics::EPosition pos,
                        double depth)
{
  m_pScreen->drawSymbol(pt, symbolName, pos, depth);
}

void Drawer::drawCircle(m2::PointD const & pt,
                        graphics::EPosition pos,
                        di::DrawRule const & rule,
                        FeatureID const & id)
{
  graphics::Circle::Info ci;
  ConvertStyle(rule.m_rule->GetCircle(), m_visualScale, ci);

  graphics::CircleElement::Params params;

  params.m_depth = rule.m_depth;
  params.m_position = pos;
  params.m_pivot = pt;
  params.m_ci = ci;
  params.m_userInfo.m_mwmID = id.m_mwmId;
  params.m_userInfo.m_offset = id.m_offset;

  m_pScreen->drawCircle(params);
}

void Drawer::drawSymbol(m2::PointD const & pt,
                        graphics::EPosition pos,
                        di::DrawRule const & rule,
                        FeatureID const & id)
{
  graphics::Icon::Info info;
  ConvertStyle(rule.m_rule->GetSymbol(), info);

  graphics::SymbolElement::Params params;
  params.m_depth = rule.m_depth;
  params.m_position = pos;
  params.m_pivot = pt;
  params.m_info = info;
  params.m_renderer = m_pScreen.get();
  params.m_userInfo.m_mwmID = id.m_mwmId;
  params.m_userInfo.m_offset = id.m_offset;

  m_pScreen->drawSymbol(params);
}

void Drawer::drawCircledSymbol(m2::PointD const & pt,
                               graphics::EPosition pos,
                               di::DrawRule const & symbolRule,
                               di::DrawRule const & circleRule,
                               FeatureID const & id)
{
  graphics::Icon::Info info;
  ConvertStyle(symbolRule.m_rule->GetSymbol(), info);

  graphics::Circle::Info ci;
  ConvertStyle(circleRule.m_rule->GetCircle(), m_visualScale, ci);

  graphics::SymbolElement::Params symParams;
  symParams.m_depth = symbolRule.m_depth;
  symParams.m_position = pos;
  symParams.m_pivot = pt;
  symParams.m_info = info;
  symParams.m_renderer = m_pScreen.get();
  symParams.m_userInfo.m_mwmID = id.m_mwmId;
  symParams.m_userInfo.m_offset = id.m_offset;

  graphics::CircleElement::Params circleParams;
  circleParams.m_depth = circleRule.m_depth;
  circleParams.m_position = pos;
  circleParams.m_pivot = pt;
  circleParams.m_ci = ci;
  circleParams.m_userInfo.m_mwmID = id.m_mwmId;
  circleParams.m_userInfo.m_offset = id.m_offset;

  m_pScreen->drawCircledSymbol(symParams, circleParams);
}

void Drawer::drawPath(di::PathInfo const & path, di::DrawRule const * rules, size_t count)
{
  // if any rule needs caching - cache as a whole vector
  bool flag = false;
  for (size_t i = 0; i < count; ++i)
  {
    if (rules[i].GetID(m_pScreen->threadSlot()) == drule::BaseRule::empty_id)
    {
      flag = true;
      break;
    }
  }

  buffer_vector<graphics::Pen::Info, 8> penInfos(count);
  buffer_vector<graphics::Resource::Info const*, 8> infos(count);
  buffer_vector<uint32_t, 8> resIDs(count);

  if (flag)
  {
    // collect graphics::PenInfo into array and pack them as a whole
    for (size_t i = 0; i < count; ++i)
    {
      ConvertStyle(rules[i].m_rule->GetLine(), m_visualScale, penInfos[i]);

      infos[i] = &penInfos[i];

      resIDs[i] = m_pScreen->invalidHandle();
    }

    // map array of pens
    if (m_pScreen->mapInfo(&infos[0], &resIDs[0], count))
    {
      for (size_t i = 0; i < count; ++i)
        rules[i].SetID(ThreadSlot(), resIDs[i]);
    }
    else
    {
      buffer_vector<m2::PointU, 8> rects;
      for (unsigned i = 0; i < count; ++i)
        rects.push_back(infos[i]->resourceSize());
      LOG(LERROR, ("couldn't successfully pack a sequence of path styles as a whole:" , rects));

      return;
    }
  }

  // draw path with array of rules
  for (size_t i = 0; i < count; ++i)
    m_pScreen->drawPath(&path.m_path[0],
                        path.m_path.size(),
                        -path.GetOffset(),
                        rules[i].GetID(ThreadSlot()),
                        rules[i].m_depth);
}

void Drawer::drawArea(di::AreaInfo const & area, di::DrawRule const & rule)
{
  // DO NOT cache 'id' in pRule, because one rule can use in drawPath and drawArea.
  // Leave CBaseRule::m_id for drawPath. mapColor working fast enough.

  graphics::Brush::Info info;
  ConvertStyle(rule.m_rule->GetArea(), info);

  uint32_t const id = m_pScreen->mapInfo(info);
  ASSERT ( id != -1, () );

  m_pScreen->drawTrianglesList(&area.m_path[0], area.m_path.size(), id, rule.m_depth);
}

void Drawer::drawText(m2::PointD const & pt,
                      graphics::EPosition pos,
                      di::FeatureStyler const & fs,
                      di::DrawRule const & rule,
                      FeatureID const & id)
{
  graphics::FontDesc primaryFont;
  m2::PointD primaryOffset;
  ConvertStyle(rule.m_rule->GetCaption(0), m_visualScale, primaryFont, primaryOffset);
  primaryFont.SetRank(fs.m_popRank);

  graphics::FontDesc secondaryFont;
  m2::PointD secondaryOffset;
  if (rule.m_rule->GetCaption(1))
  {
    ConvertStyle(rule.m_rule->GetCaption(1), m_visualScale, secondaryFont, secondaryOffset);
    secondaryFont.SetRank(fs.m_popRank);
  }

  graphics::StraightTextElement::Params params;
  params.m_depth = rule.m_depth;
  params.m_fontDesc = primaryFont;
  params.m_auxFontDesc = secondaryFont;
  params.m_offset = primaryOffset;
  params.m_log2vis = true;
  params.m_pivot = pt;
  params.m_position = pos;
  params.m_logText = strings::MakeUniString(fs.m_primaryText);
  params.m_auxLogText = strings::MakeUniString(fs.m_secondaryText);
  params.m_doSplit = true;
  params.m_useAllParts = false;
  params.m_userInfo.m_mwmID = id.m_mwmId;
  params.m_userInfo.m_offset = id.m_offset;

  m_pScreen->drawTextEx(params);
}

void Drawer::drawPathText(di::PathInfo const & path,
                          di::FeatureStyler const & fs,
                          di::DrawRule const & rule)
{
  graphics::FontDesc font;
  m2::PointD offset;
  ConvertStyle(rule.m_rule->GetCaption(0), m_visualScale, font, offset);

  if (fs.m_offsets.empty())
    return;

  m_pScreen->drawPathText(font,
                          &path.m_path[0],
                          path.m_path.size(),
                          fs.GetPathName(),
                          path.GetFullLength(),
                          path.GetOffset(),
                          &fs.m_offsets[0],
                          fs.m_offsets.size(),
                          rule.m_depth);
}

void Drawer::drawPathNumber(di::PathInfo const & path,
                            di::FeatureStyler const & fs)
{
  int const textHeight = static_cast<int>(11 * m_visualScale);
  m2::PointD pt;
  double const length = path.GetFullLength();
  if (length >= (fs.m_refText.size() + 2) * textHeight)
  {
    size_t const count = size_t(length / 1000.0) + 2;

    for (size_t j = 1; j < count; ++j)
    {
      if (path.GetSmPoint(double(j) / double(count), pt))
      {
        graphics::FontDesc fontDesc(
          textHeight,
          graphics::Color(150, 75, 0, 255),   // brown
          true,
          graphics::Color(255, 255, 255, 255));

        m_pScreen->drawText(fontDesc,
                            pt,
                            graphics::EPosCenter,
                            fs.m_refText,
                            0,
                            true);
      }
    }
  }
}

graphics::Screen * Drawer::screen() const
{
  return m_pScreen.get();
}

double Drawer::VisualScale() const
{
  return m_visualScale;
}

void Drawer::SetScale(int level)
{
  m_level = level;
}

void Drawer::Draw(di::FeatureInfo const & fi)
{
  FeatureID const & id = fi.m_id;
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
      drawPath(*i, pathRules.data(), pathRules.size());
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
            drawArea(*i, di::DrawRule(pRule, depth));
          else if (isCircleAndSymbol && isCircle)
          {
            drawCircledSymbol(i->GetCenter(),
                              graphics::EPosCenter,
                              di::DrawRule(pSymbolRule, symbolDepth),
                              di::DrawRule(pCircleRule, circleDepth),
                              id);
          }
          else if (isSymbol)
            drawSymbol(i->GetCenter(), graphics::EPosCenter, di::DrawRule(pRule, depth), id);
          else if (isCircle)
            drawCircle(i->GetCenter(), graphics::EPosCenter, di::DrawRule(pRule, depth), id);
        }
      }

      // draw point symbol
      if (!isPath && !isArea && ((pRule->GetType() & drule::node) != 0))
      {
        if (isCircleAndSymbol)
        {
          drawCircledSymbol(fi.m_point,
                            graphics::EPosCenter,
                            di::DrawRule(pSymbolRule, symbolDepth),
                            di::DrawRule(pCircleRule, circleDepth),
                            id);
        }
        else if (isSymbol)
          drawSymbol(fi.m_point, graphics::EPosCenter, di::DrawRule(pRule, depth), id);
        else if (isCircle)
          drawCircle(fi.m_point, graphics::EPosCenter, di::DrawRule(pRule, depth), id);
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
            drawText(i->GetCenter(), textPosition, fi.m_styler, di::DrawRule(pRule, depth), id);
        }

        // draw way name
        if (isPath && !isArea && isN && !fi.m_styler.FilterTextSize(pRule))
        {
          for (list<di::PathInfo>::const_iterator i = fi.m_pathes.begin(); i != fi.m_pathes.end(); ++i)
            drawPathText(*i, fi.m_styler, di::DrawRule(pRule, depth));
        }

        // draw point text
        isN = ((pRule->GetType() & drule::node) != 0);
        if (!isPath && !isArea && isN)
          drawText(fi.m_point, textPosition, fi.m_styler, di::DrawRule(pRule, depth), id);
      }
    }
  }

  // draw road numbers
  if (isPath && !fi.m_styler.m_refText.empty() && m_level >= 12)
    for (auto n : fi.m_pathes)
      drawPathNumber(n, fi.m_styler);
}

int Drawer::ThreadSlot() const
{
  return m_pScreen->threadSlot();
}
