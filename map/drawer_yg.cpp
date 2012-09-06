#include "drawer_yg.hpp"
#include "proto_to_yg_styles.hpp"

#include "../std/bind.hpp"

#include "../indexer/drawing_rules.hpp"
#include "../indexer/scales.hpp"

#include "../yg/defines.hpp"
#include "../yg/screen.hpp"
#include "../yg/skin.hpp"
#include "../yg/resource_manager.hpp"

#include "../geometry/screenbase.hpp"

#include "../base/logging.hpp"
#include "../base/buffer_vector.hpp"


DrawerYG::Params::Params()
  : m_threadID(0),
    m_visualScale(1)
{
}

di::DrawInfo::DrawInfo(string const & name,
         string const & secondaryName,
         string const & road,
         double rank)
  : m_name(name),
    m_secondaryName(secondaryName),
    m_road(road),
    m_rank(rank)
{}

string const di::DrawInfo::GetPathName() const
{
  if (m_secondaryName.empty())
    return m_name;
  else
    return m_name + "      " + m_secondaryName;
}

uint32_t di::DrawRule::GetID(size_t threadID) const
{
  return (m_transparent ? m_rule->GetID2(threadID) : m_rule->GetID(threadID));
}

void di::DrawRule::SetID(size_t threadID, uint32_t id) const
{
  m_transparent ? m_rule->SetID2(threadID, id) : m_rule->SetID(threadID, id);
}

DrawerYG::DrawerYG(params_t const & params)
  : m_visualScale(params.m_visualScale), m_threadID(params.m_threadID)
{
  m_pScreen = shared_ptr<yg::gl::Screen>(new yg::gl::Screen(params));

  m_pSkin = params.m_skin;
  m_pScreen->setSkin(m_pSkin);

  if (m_pSkin)
    m_pSkin->addClearPageFn(bind(&DrawerYG::ClearSkinPage, m_threadID, _1), 0);
}

namespace
{
  struct DoMakeInvalidRule
  {
    size_t m_threadID;
    uint32_t m_pipelineIDMask;

    DoMakeInvalidRule(size_t threadID, uint8_t pipelineID)
      : m_threadID(threadID), m_pipelineIDMask(pipelineID << 24)
    {}

    void operator() (int, int, int, drule::BaseRule * p)
    {
      if ((p->GetID(m_threadID) & 0xFF000000) == m_pipelineIDMask)
        p->MakeEmptyID(m_threadID);
      if ((p->GetID2(m_threadID) & 0xFF000000) == m_pipelineIDMask)
        p->MakeEmptyID2(m_threadID);
    }
  };
}

void DrawerYG::ClearSkinPage(size_t threadID, uint8_t pipelineID)
{
  drule::rules().ForEachRule(DoMakeInvalidRule(threadID, pipelineID));
}

void DrawerYG::beginFrame()
{
  m_pScreen->beginFrame();
}

void DrawerYG::endFrame()
{
  m_pScreen->endFrame();
}

void DrawerYG::clear(yg::Color const & c, bool clearRT, float depth, bool clearDepth)
{
  m_pScreen->clear(c, clearRT, depth, clearDepth);
}

void DrawerYG::onSize(int w, int h)
{
  m_pScreen->onSize(w, h);
}

void DrawerYG::drawSymbol(m2::PointD const & pt, string const & symbolName, yg::EPosition pos, int depth)
{
  m_pScreen->drawSymbol(pt, symbolName.c_str(), pos, depth);
}

void DrawerYG::drawCircle(m2::PointD const & pt, rule_ptr_t pRule, yg::EPosition pos, int depth)
{
  yg::CircleInfo ci;
  ConvertStyle(pRule->GetCircle(), m_visualScale, ci);

  m_pScreen->drawCircle(pt, ci, pos, depth);
}

void DrawerYG::drawSymbol(m2::PointD const & pt, rule_ptr_t pRule, yg::EPosition pos, int depth)
{
  string name;
  ConvertStyle(pRule->GetSymbol(), name);

  m_pScreen->drawSymbol(pt, name, pos, depth);
}

void DrawerYG::drawPath(di::PathInfo const & info, di::DrawRule const * rules, size_t count)
{
  // if any rule needs caching - cache as a whole vector
  bool flag = false;
  for (size_t i = 0; i < count; ++i)
  {
    if (rules[i].GetID(m_threadID) == drule::BaseRule::empty_id)
    {
      flag = true;
      break;
    }
  }

  buffer_vector<yg::PenInfo, 8> penInfos(count);
  buffer_vector<uint32_t, 8> styleIDs(count);

  if (flag)
  {
    // collect yg::PenInfo into array and pack them as a whole
    for (size_t i = 0; i < count; ++i)
    {
      ConvertStyle(rules[i].m_rule->GetLine(), m_visualScale, penInfos[i]);

      if (rules[i].m_transparent)
        penInfos[i].m_color.a = 100;

      styleIDs[i] = m_pSkin->invalidHandle();
    }

    // map array of pens
    if (m_pSkin->mapPenInfo(&penInfos[0], &styleIDs[0], count))
    {
      for (size_t i = 0; i < count; ++i)
        rules[i].SetID(m_threadID, styleIDs[i]);
    }
    else
    {
      LOG(LERROR, ("couldn't successfully pack a sequence of path styles as a whole"));
      return;
    }
  }

  // draw path with array of rules
  for (size_t i = 0; i < count; ++i)
    m_pScreen->drawPath(&info.m_path[0], info.m_path.size(), -info.GetOffset(), rules[i].GetID(m_threadID), rules[i].m_depth);
}

void DrawerYG::drawArea(vector<m2::PointD> const & pts, rule_ptr_t pRule, int depth)
{
  // DO NOT cache 'id' in pRule, because one rule can use in drawPath and drawArea.
  // Leave CBaseRule::m_id for drawPath. mapColor working fast enough.

  yg::Color color;
  ConvertStyle(pRule->GetArea(), color);
  uint32_t const id = m_pSkin->mapColor(color);
  ASSERT ( id != -1, () );

  m_pScreen->drawTrianglesList(&pts[0], pts.size()/*, res*/, id, depth);
}

uint8_t DrawerYG::get_text_font_size(rule_ptr_t pRule) const
{
  return GetFontSize(pRule->GetCaption(0)) * m_visualScale;
}

bool DrawerYG::filter_text_size(rule_ptr_t pRule) const
{
  if (pRule->GetCaption(0))
    return (GetFontSize(pRule->GetCaption(0)) < 3);
  else
  {
    // this rule is not a caption at all
    return true;
  }
}

void DrawerYG::drawText(m2::PointD const & pt, di::DrawInfo const * pInfo, rule_ptr_t pRule, yg::EPosition pos, int depth)
{
  yg::FontDesc font;
  ConvertStyle(pRule->GetCaption(0), m_visualScale, font);
  font.SetRank(pInfo->m_rank);

  yg::FontDesc smallFont;
  if (pRule->GetCaption(1))
  {
    ConvertStyle(pRule->GetCaption(1), m_visualScale, smallFont);
    smallFont.SetRank(pInfo->m_rank);
  }

  m_pScreen->drawTextEx(font, smallFont, pt, pos,
                        pInfo->m_name, pInfo->m_secondaryName,
                        depth, true, true);
}

bool DrawerYG::drawPathText(di::PathInfo const & info, di::DrawInfo const * pInfo, rule_ptr_t pRule, int depth)
{
  yg::FontDesc font;
  ConvertStyle(pRule->GetCaption(0), m_visualScale, font);

  return m_pScreen->drawPathText(font,
                                 &info.m_path[0],
                                 info.m_path.size(),
                                 pInfo->GetPathName(),
                                 info.GetFullLength(),
                                 info.GetOffset(),
                                 yg::EPosCenter,
                                 depth);
}

void DrawerYG::drawPathNumber(di::PathInfo const & path, di::DrawInfo const * pInfo)
{
  int const textHeight = static_cast<int>(12 * m_visualScale);
  m2::PointD pt;
  double const length = path.GetFullLength();
  if (length >= (pInfo->m_road.size() + 2)*textHeight)
  {
    size_t const count = size_t(length / 1000.0) + 2;

    for (size_t j = 1; j < count; ++j)
    {
      if (path.GetSmPoint(double(j) / double(count), pt))
      {
        yg::FontDesc fontDesc(
          textHeight,
          yg::Color(150, 75, 0, 255),   // brown
          true,
          yg::Color(255, 255, 255, 255));

        m_pScreen->drawText(fontDesc, pt, yg::EPosCenter, pInfo->m_road, yg::maxDepth, true);
      }
    }
  }
}

shared_ptr<yg::gl::Screen> DrawerYG::screen() const
{
  return m_pScreen;
}

double DrawerYG::VisualScale() const
{
  return m_visualScale;
}

void DrawerYG::SetScale(int level)
{
  m_level = level;
}

void DrawerYG::Draw(di::DrawInfo const * pInfo, di::DrawRule const * rules, size_t count)
{
  buffer_vector<di::DrawRule, 8> pathRules;

  bool const isPath = !pInfo->m_pathes.empty();
  bool const isArea = !pInfo->m_areas.empty();

  // separating path rules from other
  for (size_t i = 0; i < count; ++i)
  {
    rule_ptr_t pRule = rules[i].m_rule;

    bool const hasSymbol = pRule->GetSymbol() != 0;
    bool const isCaption = pRule->GetCaption(0) != 0;

    if (!isCaption && isPath && !hasSymbol && (pRule->GetLine() != 0))
      pathRules.push_back(rules[i]);
  }

  if (!pathRules.empty())
  {
    for (list<di::PathInfo>::const_iterator i = pInfo->m_pathes.begin(); i != pInfo->m_pathes.end(); ++i)
    {
      drawPath(*i, pathRules.data(), pathRules.size());
    }
  }

  bool isNumber = true;

  for (size_t i = 0; i < count; ++i)
  {
    rule_ptr_t pRule = rules[i].m_rule;
    int const depth = rules[i].m_depth;

    bool const isCaption = pRule->GetCaption(0) != 0;
    bool const hasSymbol = pRule->GetSymbol() != 0;
    bool const isCircle = pRule->GetCircle() != 0;

    if (!isCaption)
    {
      // draw area
      if (isArea)
      {
        bool const isFill = pRule->GetArea() != 0;
        bool const hasSym = hasSymbol && ((pRule->GetType() & drule::way) != 0);

        for (list<di::AreaInfo>::const_iterator i = pInfo->m_areas.begin(); i != pInfo->m_areas.end(); ++i)
        {
          if (isFill)
            drawArea(i->m_path, pRule, depth);
          else if (hasSym)
            drawSymbol(i->GetCenter() + m2::PointD(0, 2 * m_visualScale), pRule, yg::EPosUnder, depth);
        }
      }

      // draw point symbol
      if (!isPath && !isArea && ((pRule->GetType() & drule::node) != 0))
      {
        if (hasSymbol)
          drawSymbol(pInfo->m_point + m2::PointD(0, 2 * m_visualScale), pRule, yg::EPosUnder, depth);
        if (isCircle)
          drawCircle(pInfo->m_point + m2::PointD(0, 2 * m_visualScale), pRule, yg::EPosUnder, depth);
      }
    }
    else
    {
      if (!pInfo->m_name.empty())
      {
        bool isN = ((pRule->GetType() & drule::way) != 0);

        // draw area text
        if (isArea/* && isN*/)
        {
          for (list<di::AreaInfo>::const_iterator i = pInfo->m_areas.begin(); i != pInfo->m_areas.end(); ++i)
            drawText(i->GetCenter(), pInfo, pRule, yg::EPosAbove, depth);
        }

        // draw way name
        if (isPath && !isArea && isN && !filter_text_size(pRule))
        {
          for (list<di::PathInfo>::const_iterator i = pInfo->m_pathes.begin(); i != pInfo->m_pathes.end(); ++i)
            drawPathText(*i, pInfo, pRule, depth);
        }

        // draw point text
        isN = ((pRule->GetType() & drule::node) != 0);
        if (!isPath && !isArea && isN)
          drawText(pInfo->m_point, pInfo, pRule, yg::EPosAbove, depth);
      }
    }
  }

  // draw road numbers
  if (isNumber && isPath && !pInfo->m_road.empty() && m_level >= 12)
  {
    for (list<di::PathInfo>::const_iterator i = pInfo->m_pathes.begin(); i != pInfo->m_pathes.end(); ++i)
      drawPathNumber(*i, pInfo);
  }
}
